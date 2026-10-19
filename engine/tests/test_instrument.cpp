#include <gtest/gtest.h>

#include "me/instrument.hpp"

namespace me {
namespace {

TEST(InstrumentRegistry, FindsRegisteredInstruments) {
    InstrumentRegistry instruments;
    EXPECT_TRUE(instruments.add({.symbol = "AAPL"}));
    EXPECT_TRUE(instruments.add({.symbol = "EURUSD", .price_decimals = 5}));

    const Instrument* eurusd = instruments.find("EURUSD");
    ASSERT_NE(eurusd, nullptr);
    EXPECT_EQ(eurusd->symbol, "EURUSD");
    EXPECT_EQ(eurusd->price_decimals, 5);
    EXPECT_EQ(instruments.size(), 2u);
}

TEST(InstrumentRegistry, ReturnsNullForUnknownSymbol) {
    InstrumentRegistry instruments;
    EXPECT_TRUE(instruments.add({.symbol = "AAPL"}));
    EXPECT_EQ(instruments.find("MSFT"), nullptr);
}

TEST(InstrumentRegistry, RejectsDuplicateSymbolAndKeepsTheOriginal) {
    InstrumentRegistry instruments;
    EXPECT_TRUE(instruments.add({.symbol = "AAPL", .tick_size = 1}));
    EXPECT_FALSE(instruments.add({.symbol = "AAPL", .tick_size = 5}));

    ASSERT_NE(instruments.find("AAPL"), nullptr);
    EXPECT_EQ(instruments.find("AAPL")->tick_size, 1);
    EXPECT_EQ(instruments.size(), 1u);
}

TEST(InstrumentRegistry, RejectsInvalidSettings) {
    InstrumentRegistry instruments;
    EXPECT_FALSE(instruments.add({.symbol = ""}));
    EXPECT_FALSE(instruments.add({.symbol = "A", .price_decimals = -1}));
    EXPECT_FALSE(instruments.add({.symbol = "B", .price_decimals = kMaxPriceDecimals + 1}));
    EXPECT_FALSE(instruments.add({.symbol = "C", .tick_size = 0}));
    EXPECT_FALSE(instruments.add({.symbol = "D", .lot_size = -1}));
    EXPECT_FALSE(instruments.add({.symbol = "E", .max_order_qty = 0}));
    EXPECT_EQ(instruments.size(), 0u);
}

}  // namespace
}  // namespace me
