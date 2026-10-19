#include <gtest/gtest.h>

#include <limits>
#include <string_view>

#include "me/price.hpp"

namespace me {
namespace {

TEST(ParsePrice, ParsesWholeAndFractionalParts) {
    EXPECT_EQ(parse_price("101.25"), 10125);
    EXPECT_EQ(parse_price("101.5"), 10150);
    EXPECT_EQ(parse_price("101"), 10100);
    EXPECT_EQ(parse_price("0.01"), 1);
    EXPECT_EQ(parse_price("0"), 0);
    EXPECT_EQ(parse_price("007.10"), 710);
}

TEST(ParsePrice, ParsesNegativePrices) {
    EXPECT_EQ(parse_price("-0.50"), -50);
    EXPECT_EQ(parse_price("-3"), -300);
}

TEST(ParsePrice, AcceptsTrailingZerosBeyondPrecision) {
    EXPECT_EQ(parse_price("101.250"), 10125);
    EXPECT_EQ(parse_price("101.2500000"), 10125);
}

TEST(ParsePrice, RejectsPricesMorePreciseThanAllowed) {
    EXPECT_EQ(parse_price("101.255"), std::nullopt);
    EXPECT_EQ(parse_price("0.001"), std::nullopt);
}

TEST(ParsePrice, RejectsMalformedText) {
    for (const std::string_view text : {"", "-", ".", "1.", ".5", "-.5", "abc", "1.2.3", "1,000",
                                        " 1", "1 ", "+1", "--1", "1e3"}) {
        EXPECT_EQ(parse_price(text), std::nullopt) << '"' << text << '"';
    }
}

TEST(ParsePrice, RejectsValuesThatOverflow) {
    // The largest Price is 9223372036854775807, which is 92233720368547758.07
    // with 2 decimals.
    EXPECT_EQ(parse_price("92233720368547758.07"), std::numeric_limits<Price>::max());
    EXPECT_EQ(parse_price("92233720368547758.08"), std::nullopt);
    EXPECT_EQ(parse_price("100000000000000000000"), std::nullopt);
}

TEST(FormatPrice, PadsTheFraction) {
    EXPECT_EQ(format_price(10125), "101.25");
    EXPECT_EQ(format_price(100), "1.00");
    EXPECT_EQ(format_price(5), "0.05");
    EXPECT_EQ(format_price(0), "0.00");
    EXPECT_EQ(format_price(-5), "-0.05");
}

TEST(FormatPrice, HandlesExtremes) {
    EXPECT_EQ(format_price(std::numeric_limits<Price>::max()), "92233720368547758.07");
    EXPECT_EQ(format_price(std::numeric_limits<Price>::min()), "-92233720368547758.08");
}

TEST(PriceConversion, RoundTripsCanonicalStrings) {
    for (const std::string_view text : {"101.25", "0.01", "0.00", "99999.99", "-12.34"}) {
        const auto price = parse_price(text);
        ASSERT_TRUE(price.has_value()) << text;
        EXPECT_EQ(format_price(*price), text);
    }
}

TEST(PriceConversion, SupportsOtherPrecisions) {
    EXPECT_EQ(parse_price("1.08455", 5), 108455);
    EXPECT_EQ(format_price(108455, 5), "1.08455");

    EXPECT_EQ(parse_price("7", 0), 7);
    EXPECT_EQ(parse_price("7.0", 0), 7);
    EXPECT_EQ(parse_price("7.5", 0), std::nullopt);
    EXPECT_EQ(format_price(7, 0), "7");
}

}  // namespace
}  // namespace me
