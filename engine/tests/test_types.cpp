#include <gtest/gtest.h>

#include "me/order.hpp"
#include "me/types.hpp"
#include "printers.hpp"

namespace me {
namespace {

// The enum helpers are constexpr, so they can be checked at compile time.
static_assert(opposite(Side::Buy) == Side::Sell);
static_assert(opposite(Side::Sell) == Side::Buy);
static_assert(to_string(Side::Buy) == "BUY");
static_assert(to_string(Side::Sell) == "SELL");
static_assert(to_string(OrderType::Limit) == "LIMIT");
static_assert(to_string(OrderType::Market) == "MARKET");
static_assert(to_string(TimeInForce::GTC) == "GTC");
static_assert(to_string(TimeInForce::IOC) == "IOC");
static_assert(to_string(TimeInForce::FOK) == "FOK");

TEST(Order, DefaultsToGtcLimitOrder) {
    const Order order;
    EXPECT_EQ(order.type, OrderType::Limit);
    EXPECT_EQ(order.tif, TimeInForce::GTC);
    EXPECT_EQ(order.filled_qty, 0);
}

TEST(Order, RemainingQuantityTracksFills) {
    Order order{.id = 1, .symbol = "AAPL", .side = Side::Buy, .price = 18950, .qty = 100};
    EXPECT_EQ(order.remaining_qty(), 100);
    EXPECT_FALSE(order.is_filled());

    order.filled_qty = 40;
    EXPECT_EQ(order.remaining_qty(), 60);
    EXPECT_FALSE(order.is_filled());

    order.filled_qty = 100;
    EXPECT_EQ(order.remaining_qty(), 0);
    EXPECT_TRUE(order.is_filled());
}

TEST(Order, DescribesLimitOrder) {
    const Order order{.id = 7, .symbol = "AAPL", .side = Side::Buy, .price = 18950, .qty = 100};
    EXPECT_EQ(to_string(order), "#7 AAPL BUY 100 @ 189.50 GTC");
}

TEST(Order, DescribesPartlyFilledMarketOrder) {
    const Order order{.id = 8,
                      .symbol = "AAPL",
                      .side = Side::Sell,
                      .type = OrderType::Market,
                      .tif = TimeInForce::IOC,
                      .qty = 50,
                      .filled_qty = 20};
    EXPECT_EQ(to_string(order), "#8 AAPL SELL 50 @ MKT IOC (filled 20)");
}

TEST(Order, DescribesPriceWithTheGivenDecimals) {
    const Order order{.id = 9, .symbol = "EURUSD", .side = Side::Buy, .price = 108455, .qty = 1000};
    EXPECT_EQ(to_string(order, 5), "#9 EURUSD BUY 1000 @ 1.08455 GTC");
}

}  // namespace
}  // namespace me
