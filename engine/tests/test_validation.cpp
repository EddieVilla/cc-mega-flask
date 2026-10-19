#include <gtest/gtest.h>

#include "me/validation.hpp"
#include "printers.hpp"

namespace me {
namespace {

class ValidationTest : public ::testing::Test {
protected:
    // Prices in cents with a 0.05 tick; quantities in round lots of 10, at
    // most 1000 per order.
    const Instrument demo_{.symbol = "DEMO", .tick_size = 5, .lot_size = 10, .max_order_qty = 1000};

    static Order limit(Side side, Qty qty, Price price) {
        return {.id = 1, .symbol = "DEMO", .side = side, .price = price, .qty = qty};
    }

    static Order market(Side side, Qty qty) {
        return {.id = 1,
                .symbol = "DEMO",
                .side = side,
                .type = OrderType::Market,
                .tif = TimeInForce::IOC,
                .qty = qty};
    }
};

TEST_F(ValidationTest, AcceptsValidLimitOrders) {
    EXPECT_EQ(validate(limit(Side::Buy, 100, 10025), demo_), std::nullopt);
    EXPECT_EQ(validate(limit(Side::Sell, 10, 5), demo_), std::nullopt);
}

TEST_F(ValidationTest, AcceptsMarketOrderWithoutPrice) {
    EXPECT_EQ(validate(market(Side::Sell, 50), demo_), std::nullopt);
}

TEST_F(ValidationTest, RejectsZeroOrNegativeQuantity) {
    EXPECT_EQ(validate(limit(Side::Buy, 0, 10025), demo_), RejectReason::InvalidQuantity);
    EXPECT_EQ(validate(limit(Side::Buy, -10, 10025), demo_), RejectReason::InvalidQuantity);
    EXPECT_EQ(validate(market(Side::Buy, 0), demo_), RejectReason::InvalidQuantity);
}

TEST_F(ValidationTest, RejectsQuantityAboveMaximum) {
    EXPECT_EQ(validate(limit(Side::Buy, 1000, 10025), demo_), std::nullopt);
    EXPECT_EQ(validate(limit(Side::Buy, 1010, 10025), demo_), RejectReason::QuantityTooLarge);
    EXPECT_EQ(validate(market(Side::Sell, 1010), demo_), RejectReason::QuantityTooLarge);
}

TEST_F(ValidationTest, RejectsQuantityThatIsNotWholeLots) {
    EXPECT_EQ(validate(limit(Side::Buy, 15, 10025), demo_), RejectReason::InvalidLotSize);
}

TEST_F(ValidationTest, RejectsLimitOrderWithoutPositivePrice) {
    EXPECT_EQ(validate(limit(Side::Buy, 10, 0), demo_), RejectReason::InvalidPrice);
    EXPECT_EQ(validate(limit(Side::Sell, 10, -5), demo_), RejectReason::InvalidPrice);
}

TEST_F(ValidationTest, RejectsMarketOrderWithPrice) {
    Order order = market(Side::Buy, 10);
    order.price = 10025;
    EXPECT_EQ(validate(order, demo_), RejectReason::InvalidPrice);
}

TEST_F(ValidationTest, RejectsPriceOffTick) {
    // 100.27 is not a multiple of 0.05; 100.30 is.
    EXPECT_EQ(validate(limit(Side::Buy, 10, 10027), demo_), RejectReason::PriceNotOnTick);
    EXPECT_EQ(validate(limit(Side::Buy, 10, 10030), demo_), std::nullopt);
}

TEST_F(ValidationTest, RejectsOrderForAnotherInstrument) {
    Order order = limit(Side::Buy, 10, 10025);
    order.symbol = "OTHER";
    EXPECT_EQ(validate(order, demo_), RejectReason::UnknownSymbol);
}

TEST(ValidateWithRegistry, RejectsUnknownSymbol) {
    InstrumentRegistry instruments;
    ASSERT_TRUE(instruments.add({.symbol = "DEMO"}));

    const Order known{.id = 1, .symbol = "DEMO", .price = 10000, .qty = 10};
    Order unknown = known;
    unknown.symbol = "NOPE";

    EXPECT_EQ(validate(known, instruments), std::nullopt);
    EXPECT_EQ(validate(unknown, instruments), RejectReason::UnknownSymbol);
}

TEST(ValidateWithRegistry, UsesEachSymbolsOwnRules) {
    InstrumentRegistry instruments;
    ASSERT_TRUE(instruments.add({.symbol = "PENNY", .tick_size = 1}));
    ASSERT_TRUE(instruments.add({.symbol = "NICKEL", .tick_size = 5}));

    const Order penny{.id = 1, .symbol = "PENNY", .price = 10027, .qty = 10};
    const Order nickel{.id = 2, .symbol = "NICKEL", .price = 10027, .qty = 10};

    EXPECT_EQ(validate(penny, instruments), std::nullopt);
    EXPECT_EQ(validate(nickel, instruments), RejectReason::PriceNotOnTick);
}

}  // namespace
}  // namespace me
