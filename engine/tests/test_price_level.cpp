#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "me/price_level.hpp"
#include "printers.hpp"

namespace me {
namespace {

using ::testing::ElementsAre;

Order bid(OrderId id, Qty qty) {
    return {.id = id, .symbol = "DEMO", .side = Side::Buy, .price = 10000, .qty = qty, .seq = id};
}

std::vector<OrderId> ids(const PriceLevel& level) {
    std::vector<OrderId> result;
    for (const Order& order : level) result.push_back(order.id);
    return result;
}

TEST(PriceLevel, StartsEmpty) {
    const PriceLevel level(10000);
    EXPECT_EQ(level.price(), 10000);
    EXPECT_TRUE(level.empty());
    EXPECT_EQ(level.total_qty(), 0);
    EXPECT_EQ(level.order_count(), 0u);
}

TEST(PriceLevel, QueuesOrdersInArrivalOrder) {
    PriceLevel level(10000);
    level.push_back(bid(1, 100));
    level.push_back(bid(2, 200));
    level.push_back(bid(3, 300));

    EXPECT_THAT(ids(level), ElementsAre(1, 2, 3));
    EXPECT_EQ(level.front().id, 1u);
    EXPECT_EQ(level.total_qty(), 600);
    EXPECT_EQ(level.order_count(), 3u);
}

TEST(PriceLevel, RemoveTakesOutOneOrderAndReturnsIt) {
    PriceLevel level(10000);
    level.push_back(bid(1, 100));
    const auto middle = level.push_back(bid(2, 200));
    level.push_back(bid(3, 300));

    EXPECT_EQ(level.remove(middle), bid(2, 200));
    EXPECT_THAT(ids(level), ElementsAre(1, 3));
    EXPECT_EQ(level.total_qty(), 400);
}

TEST(PriceLevel, TotalCountsOnlyRemainingQuantity) {
    PriceLevel level(10000);
    Order partly_filled = bid(1, 100);
    partly_filled.filled_qty = 30;

    const auto it = level.push_back(partly_filled);
    EXPECT_EQ(level.total_qty(), 70);

    level.remove(it);
    EXPECT_EQ(level.total_qty(), 0);
    EXPECT_TRUE(level.empty());
}

TEST(PriceLevel, IteratorsStayValidWhenOtherOrdersLeave) {
    PriceLevel level(10000);
    const auto first = level.push_back(bid(1, 100));
    const auto second = level.push_back(bid(2, 200));
    const auto last = level.push_back(bid(3, 300));

    level.remove(first);
    level.remove(second);
    EXPECT_EQ(level.remove(last).id, 3u);
    EXPECT_TRUE(level.empty());
}

}  // namespace
}  // namespace me
