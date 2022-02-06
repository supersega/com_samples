#include <ComUtility/Utility.h>
#include <ComUtility/SynchronizationContext.h>
#include <gtest/gtest.h>

TEST(ComUtilityTest,
    RequireThat_RaiseSystemError_ThrowsSystemError)
{
    EXPECT_THROW(RaiseSystemError(5, "Some failure"), std::system_error);
}

TEST(DispatcherTest,
    RequireThat_Send_ReturnsResultOfCall_WhenCalled)
{
    SynchronizationContext ds;
    EXPECT_EQ(ds.Send([](auto a, auto b) { return a + b; }, 1, 2), 3);
}

TEST(DispatcherTest,
    RequireThat_Post_ReturnsResultOfCall_WhenCalled)
{
    SynchronizationContext ds;
    EXPECT_EQ(ds.Post([](auto a, auto b) { return a + b; }, 1, 2).get(), 3);
}
