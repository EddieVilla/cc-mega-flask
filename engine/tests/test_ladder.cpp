#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "me/ladder.hpp"
#include "me/order_book.hpp"

namespace me {
namespace {

using ::testing::HasSubstr;
using ::testing::Not;

class LadderTest : public ::testing::Test {
protected:
    OrderBook book_{Instrument{.symbol = "DEMO"}};
    OrderId next_id_ = 1;

    void add(Side side, Qty qty, Price price) {
        const OrderId id = next_id_++;
        ASSERT_TRUE(book_.add_resting(
            {.id = id, .symbol = "DEMO", .side = side, .price = price, .qty = qty, .seq = id}));
    }

    // Two bid levels and two ask levels, with several orders at some prices.
    void add_two_sided_book() {
        add(Side::Buy, 100, 10002);
        add(Side::Buy, 200, 10002);
        add(Side::Buy, 150, 10001);
        add(Side::Sell, 200, 10005);
        add(Side::Sell, 100, 10006);
        add(Side::Sell, 150, 10006);
        add(Side::Sell, 250, 10006);
    }
};

TEST_F(LadderTest, RendersEmptyBook) {
    EXPECT_EQ(render_ladder(book_),
              "DEMO  orders: 0  bid levels: 0  ask levels: 0\n"
              "  side       price         qty  orders\n"
              "--------------------------------------  spread n/a\n");
}

TEST_F(LadderTest, RendersAsksAboveBidsWithBestPricesInTheMiddle) {
    add_two_sided_book();
    EXPECT_EQ(render_ladder(book_),
              "DEMO  orders: 7  bid levels: 2  ask levels: 2\n"
              "  side       price         qty  orders\n"
              "   ASK      100.06         500       3\n"
              "   ASK      100.05         200       1\n"
              "--------------------------------------  spread 0.03\n"
              "   BID      100.02         300       2\n"
              "   BID      100.01         150       1\n");
}

TEST_F(LadderTest, ShowsOnlyTheBestLevelsWhenLimited) {
    add_two_sided_book();
    EXPECT_EQ(render_ladder(book_, 1),
              "DEMO  orders: 7  bid levels: 2  ask levels: 2\n"
              "  side       price         qty  orders\n"
              "   ASK      100.05         200       1\n"
              "--------------------------------------  spread 0.03\n"
              "   BID      100.02         300       2\n");
}

TEST_F(LadderTest, RendersOneSidedBook) {
    add(Side::Buy, 100, 10000);
    EXPECT_EQ(render_ladder(book_),
              "DEMO  orders: 1  bid levels: 1  ask levels: 0\n"
              "  side       price         qty  orders\n"
              "--------------------------------------  spread n/a\n"
              "   BID      100.00         100       1\n");
}

TEST_F(LadderTest, FlagsLockedAndCrossedBooks) {
    add(Side::Buy, 100, 10000);
    add(Side::Sell, 100, 10000);
    EXPECT_THAT(render_ladder(book_), HasSubstr("spread 0.00 (locked)"));

    add(Side::Buy, 100, 10002);
    EXPECT_THAT(render_ladder(book_), HasSubstr("spread -0.02 (crossed)"));
    EXPECT_THAT(render_ladder(book_), Not(HasSubstr("locked")));
}

TEST(Ladder, FormatsPricesWithTheInstrumentsDecimals) {
    OrderBook book{Instrument{.symbol = "EURUSD", .price_decimals = 5}};
    ASSERT_TRUE(book.add_resting(
        {.id = 1, .symbol = "EURUSD", .side = Side::Buy, .price = 108455, .qty = 1000}));

    EXPECT_THAT(render_ladder(book), HasSubstr("   BID     1.08455        1000       1\n"));
}

}  // namespace
}  // namespace me
