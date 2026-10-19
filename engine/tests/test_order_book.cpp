#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <type_traits>
#include <utility>
#include <vector>

#include "me/order_book.hpp"
#include "printers.hpp"

namespace me {
namespace {

using ::testing::ElementsAre;
using ::testing::IsEmpty;

static_assert(!std::is_copy_constructible_v<OrderBook>);
static_assert(std::is_move_constructible_v<OrderBook>);

class OrderBookTest : public ::testing::Test {
protected:
    OrderBook book_{Instrument{.symbol = "DEMO"}};
    OrderId next_id_ = 1;

    static Order make_order(OrderId id, Side side, Qty qty, Price price) {
        return {.id = id, .symbol = "DEMO", .side = side, .price = price, .qty = qty, .seq = id};
    }

    // Rests a new limit order and returns its id.
    OrderId add(Side side, Qty qty, Price price) {
        const OrderId id = next_id_++;
        EXPECT_TRUE(book_.add_resting(make_order(id, side, qty, price)));
        return id;
    }

    // Ids of the orders queued at one price, front first.
    std::vector<OrderId> queue_at(Side side, Price price) const {
        std::vector<OrderId> ids;
        if (const PriceLevel* level = book_.find_level(side, price)) {
            for (const Order& order : *level) ids.push_back(order.id);
        }
        return ids;
    }
};

TEST_F(OrderBookTest, EmptyBookHasNoPrices) {
    EXPECT_TRUE(book_.empty());
    EXPECT_EQ(book_.order_count(), 0u);
    EXPECT_FALSE(book_.best_bid().has_value());
    EXPECT_FALSE(book_.best_ask().has_value());
    EXPECT_FALSE(book_.spread().has_value());
    EXPECT_THAT(book_.depth(Side::Buy, 10), IsEmpty());
    EXPECT_THAT(book_.depth(Side::Sell, 10), IsEmpty());
}

TEST_F(OrderBookTest, BestBidIsHighestBidPrice) {
    add(Side::Buy, 100, 10000);
    add(Side::Buy, 100, 10100);
    add(Side::Buy, 100, 9900);

    EXPECT_EQ(book_.best_bid(), 10100);
    EXPECT_FALSE(book_.best_ask().has_value());
    EXPECT_EQ(book_.level_count(Side::Buy), 3u);
}

TEST_F(OrderBookTest, BestAskIsLowestAskPrice) {
    add(Side::Sell, 100, 10100);
    add(Side::Sell, 100, 10000);
    add(Side::Sell, 100, 10200);

    EXPECT_EQ(book_.best_ask(), 10000);
    EXPECT_FALSE(book_.best_bid().has_value());
}

TEST_F(OrderBookTest, SpreadIsBestAskMinusBestBid) {
    add(Side::Buy, 100, 10002);
    add(Side::Sell, 100, 10005);
    EXPECT_EQ(book_.spread(), 3);
}

TEST_F(OrderBookTest, CrossingOrdersRestUntilMatchingExists) {
    add(Side::Sell, 100, 10005);
    add(Side::Buy, 100, 10007);

    EXPECT_EQ(book_.order_count(), 2u);
    EXPECT_EQ(book_.spread(), -2);
}

TEST_F(OrderBookTest, OrdersAtSamePriceQueueInArrivalOrder) {
    const OrderId first = add(Side::Buy, 100, 10000);
    const OrderId second = add(Side::Buy, 200, 10000);
    const OrderId third = add(Side::Buy, 300, 10000);

    EXPECT_THAT(queue_at(Side::Buy, 10000), ElementsAre(first, second, third));
}

TEST_F(OrderBookTest, LevelAddsUpRemainingQuantity) {
    add(Side::Buy, 100, 10000);
    add(Side::Buy, 200, 10000);
    Order partly_filled = make_order(next_id_++, Side::Buy, 100, 10000);
    partly_filled.filled_qty = 40;
    ASSERT_TRUE(book_.add_resting(partly_filled));

    EXPECT_THAT(book_.depth(Side::Buy, 1), ElementsAre(DepthLevel{10000, 360, 3}));
}

TEST_F(OrderBookTest, CancelRemovesOrderAndUpdatesTotals) {
    const OrderId first = add(Side::Buy, 100, 10000);
    const OrderId middle = add(Side::Buy, 200, 10000);
    const OrderId last = add(Side::Buy, 300, 10000);

    ASSERT_TRUE(book_.cancel(middle).has_value());

    EXPECT_THAT(queue_at(Side::Buy, 10000), ElementsAre(first, last));
    EXPECT_THAT(book_.depth(Side::Buy, 1), ElementsAre(DepthLevel{10000, 400, 2}));
    EXPECT_EQ(book_.find(middle), nullptr);
    EXPECT_EQ(book_.order_count(), 2u);
}

TEST_F(OrderBookTest, CancelReturnsTheCancelledOrder) {
    const OrderId id = add(Side::Sell, 250, 10010);
    EXPECT_EQ(book_.cancel(id), make_order(id, Side::Sell, 250, 10010));
}

TEST_F(OrderBookTest, CancellingLastOrderAtPriceRemovesTheLevel) {
    const OrderId best = add(Side::Buy, 100, 10000);
    add(Side::Buy, 100, 9900);

    ASSERT_TRUE(book_.cancel(best).has_value());

    EXPECT_EQ(book_.find_level(Side::Buy, 10000), nullptr);
    EXPECT_EQ(book_.level_count(Side::Buy), 1u);
    EXPECT_EQ(book_.best_bid(), 9900);
}

TEST_F(OrderBookTest, CancellingEverythingEmptiesTheBook) {
    const OrderId bid = add(Side::Buy, 100, 10000);
    const OrderId ask = add(Side::Sell, 100, 10100);

    ASSERT_TRUE(book_.cancel(bid).has_value());
    ASSERT_TRUE(book_.cancel(ask).has_value());

    EXPECT_TRUE(book_.empty());
    EXPECT_EQ(book_.level_count(Side::Buy), 0u);
    EXPECT_EQ(book_.level_count(Side::Sell), 0u);
}

TEST_F(OrderBookTest, CancelUnknownIdReturnsNothing) {
    const OrderId id = add(Side::Buy, 100, 10000);

    EXPECT_FALSE(book_.cancel(999).has_value());
    EXPECT_TRUE(book_.cancel(id).has_value());
    EXPECT_FALSE(book_.cancel(id).has_value());  // already gone
}

TEST_F(OrderBookTest, RejectsDuplicateOrderIdAndKeepsTheOriginal) {
    const OrderId id = add(Side::Buy, 100, 10000);

    EXPECT_FALSE(book_.add_resting(make_order(id, Side::Sell, 50, 10100)));

    EXPECT_EQ(book_.order_count(), 1u);
    EXPECT_EQ(book_.level_count(Side::Sell), 0u);
    ASSERT_NE(book_.find(id), nullptr);
    EXPECT_EQ(book_.find(id)->price, 10000);
}

TEST_F(OrderBookTest, DepthListsBestLevelsFirst) {
    add(Side::Buy, 100, 9900);
    add(Side::Buy, 200, 10100);
    add(Side::Buy, 300, 10000);
    add(Side::Buy, 400, 9800);
    add(Side::Buy, 50, 10000);
    add(Side::Sell, 500, 10300);
    add(Side::Sell, 600, 10200);

    EXPECT_THAT(book_.depth(Side::Buy, 3),
                ElementsAre(DepthLevel{10100, 200, 1}, DepthLevel{10000, 350, 2},
                            DepthLevel{9900, 100, 1}));
    EXPECT_THAT(book_.depth(Side::Sell, 10),
                ElementsAre(DepthLevel{10200, 600, 1}, DepthLevel{10300, 500, 1}));
    EXPECT_THAT(book_.depth(Side::Buy, 0), IsEmpty());
}

TEST_F(OrderBookTest, BidsAndAsksAtSamePriceAreSeparate) {
    const OrderId bid = add(Side::Buy, 100, 10000);
    const OrderId ask = add(Side::Sell, 200, 10000);

    EXPECT_THAT(queue_at(Side::Buy, 10000), ElementsAre(bid));
    EXPECT_THAT(queue_at(Side::Sell, 10000), ElementsAre(ask));

    ASSERT_TRUE(book_.cancel(bid).has_value());
    EXPECT_EQ(book_.best_ask(), 10000);
    EXPECT_FALSE(book_.best_bid().has_value());
}

TEST_F(OrderBookTest, FindReturnsTheRestingOrder) {
    const OrderId id = add(Side::Sell, 75, 10050);

    const Order* order = book_.find(id);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(*order, make_order(id, Side::Sell, 75, 10050));
    EXPECT_EQ(book_.find(id + 1), nullptr);
}

TEST_F(OrderBookTest, ReAddedOrderGoesToTheBackOfTheQueue) {
    const OrderId first = add(Side::Buy, 100, 10000);
    const OrderId second = add(Side::Buy, 100, 10000);

    ASSERT_TRUE(book_.cancel(first).has_value());
    const OrderId replacement = add(Side::Buy, 100, 10000);

    EXPECT_THAT(queue_at(Side::Buy, 10000), ElementsAre(second, replacement));
}

TEST_F(OrderBookTest, MovedBookKeepsWorking) {
    const OrderId bid = add(Side::Buy, 100, 10000);
    add(Side::Buy, 200, 9900);
    add(Side::Sell, 300, 10100);

    OrderBook moved = std::move(book_);

    // The index's iterators must now point into `moved`.
    ASSERT_TRUE(moved.cancel(bid).has_value());
    EXPECT_EQ(moved.best_bid(), 9900);
    EXPECT_EQ(moved.best_ask(), 10100);
    EXPECT_EQ(moved.order_count(), 2u);
}

}  // namespace
}  // namespace me
