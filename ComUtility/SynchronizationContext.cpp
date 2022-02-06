#include "pch.h"
#include "Include/ComUtility/SynchronizationContext.h"
#include "Include/ComUtility/Utility.h"
#include <ctxtcall.h>
#include <comsvcs.h>
#include <atlcom.h>

#include <atomic>
#include <thread>
#include <condition_variable>
#include <queue>

struct SynchronizationContext::Context
{
    Context()
    {
        HR(CoGetObjectContext(IID_PPV_ARGS(&m_callback)));
        m_poster = std::thread([this]
        {
            // Using std::thread instead async gives guaranty that COM initialized as we want
            ComRuntime rt(Apartment::MultiThreaded);

            while (true)
            {
                // Optimization to avoid lock m_messages_mutex if we if we was notified about cancellation in this point
                if (m_cancel.load())
                    break;

                std::unique_lock<std::mutex> lock(m_messages_mutex);
                // Let us wait for elements or cancellation, this will give OS chance to use thread efficiently
                m_wakeup.wait(lock, [=] { return !m_messages.empty() || m_cancel.load(std::memory_order_relaxed); });
                // If cancellation requested, then we done (std::cancellation_token is C++ 20)
                // We can make load relaxed here, because we already did check under mutex in wait
                if (m_cancel.load(std::memory_order_relaxed))
                    break;

                auto task = std::move(m_messages.front());
                m_messages.pop();

                // We unlock mutex because we don't want to block other threads from pushing tasks to the queue
                lock.unlock();

                HR(InvokeOnContext(std::move(task)));
            }
        });
    }

    ~Context()
    {
        // Before join post cancellation to exit from message loop
        // Do it under mutex to avoid deadlocks see: 
        // http://www.modernescpp.com/index.php/c-core-guidelines-be-aware-of-the-traps-of-condition-variables and
        // https://stackoverflow.com/questions/36126286/using-stdcondition-variable-with-atomicbool
        {
            std::unique_lock<std::mutex> lock(m_messages_mutex);
            m_cancel.store(true, std::memory_order_relaxed);
        }
        m_wakeup.notify_one();
        m_poster.join();
    }

    void Schedule(std::function<void()>&& fn)
    {
        {
            std::unique_lock<std::mutex> lock(m_messages_mutex);
            m_messages.push(std::move(fn));
        }
        // Notify that we should wakeup to process message
        m_wakeup.notify_one();
    }

    HRESULT InvokeOnContext(std::function<void()>&& fn)
    {
        ComCallData data;
        data.pUserDefined = &fn;
        return m_callback->ContextCallback(
            [](ComCallData* data) {
                auto& fn = *static_cast<std::function<void()>*>(data->pUserDefined);
                fn();
                return S_OK;
            }, &data, IID_IEnterActivityWithNoLock, 5, nullptr);
    }

    ATL::CComPtr<IContextCallback> m_callback;
    std::thread m_poster;
    std::queue<std::function<void()>> m_messages;
    std::mutex m_messages_mutex;
    std::condition_variable m_wakeup;
    std::atomic_bool m_cancel = false;
    std::thread::id m_thread_id = std::this_thread::get_id();
};

SynchronizationContext::SynchronizationContext() : m_context(std::make_unique<SynchronizationContext::Context>()) { }

SynchronizationContext::~SynchronizationContext() = default;

void SynchronizationContext::PostImpl(std::function<void()>&& fn) const
{
    // We can directly invoke if this is our thread
    if (m_context->m_thread_id == std::this_thread::get_id())
        fn();
    else
        m_context->Schedule(std::move(fn));
}
