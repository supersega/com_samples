#pragma once
#include <memory>
#include <type_traits>
#include <functional>
#include <tuple>
#include <future>

/** SynchronizationContext class to invoke function on dispatcher thread */
class SynchronizationContext final
{
public:
    SynchronizationContext();
    ~SynchronizationContext();

    /** Blocking function Send to invoke callable on the dispatcher thread*/
    template<typename Fn, typename... Args>
    auto Send(Fn&& fn, Args&&... args) const -> std::invoke_result_t<Fn, Args...>
    {
        // Send function can be better in C++ 20 with atomic wait for result,
        // and aligned_storage for result to avoid heap allocation in promise from Post
        return Post(std::forward<Fn>(fn), std::forward<Args>(args)...).get();
    }

    /** Non-Blocking function Post to invoke callable on the dispatcher thread*/
    template<typename Fn, typename... Args>
    auto Post(Fn fn, Args... args) const -> std::future<std::invoke_result_t<Fn, Args...>>
    {
        using result_t = std::invoke_result_t<Fn, Args...>;
        static_assert(std::is_move_constructible_v<result_t> || std::is_void_v<result_t>, "SynchronizationContext::Post - move constructor is required for 'fn(args...)' -> result return result");

        // std::shared_ptr is used because we can't move promise to extend lifetime out this function
        // Here we have at least two heap allocations because of C++ 17 limitations
        std::shared_ptr<std::promise<result_t>> promise = std::make_shared<std::promise<result_t>>();
        auto future = promise->get_future();

        // In C++ 23 we have std::move_only_function then we can
        auto task = std::function<void()>([promise = std::move(promise), fn = std::move(fn), args = std::tuple(args...)]
            {
                try
                {
                    if constexpr (!std::is_void_v<result_t>)
                        promise->set_value(std::apply(fn, args));
                    // Special case for void
                    else
                    {
                        std::apply(fn, args);
                        promise->set_value();
                    }
                }
                catch (...)
                {
                     try 
                     {
                         promise->set_exception(std::current_exception());
                     }
                     catch (...) { } // set_exception() may throw too
                }
            });
        PostImpl(std::move(task));

        return future;
    }
private:
    // std::function does not handle move only types
    void PostImpl(std::function<void()>&& fn) const;

    struct Context;
    std::unique_ptr<Context> m_context;
};
