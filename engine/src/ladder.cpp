#include "me/ladder.hpp"

#include <iomanip>
#include <ostream>
#include <sstream>
#include <string_view>

#include "me/price.hpp"

namespace me {
namespace {

constexpr int kSideWidth = 6;
constexpr int kPriceWidth = 12;
constexpr int kQtyWidth = 12;
constexpr int kOrdersWidth = 8;
constexpr std::size_t kTableWidth = kSideWidth + kPriceWidth + kQtyWidth + kOrdersWidth;

void write_row(std::ostream& out, std::string_view side, std::string_view price,
               std::string_view qty, std::string_view orders) {
    out << std::right << std::setw(kSideWidth) << side << std::setw(kPriceWidth) << price
        << std::setw(kQtyWidth) << qty << std::setw(kOrdersWidth) << orders << '\n';
}

void write_level(std::ostream& out, std::string_view side, const DepthLevel& level, int decimals) {
    write_row(out, side, format_price(level.price, decimals), std::to_string(level.total_qty),
              std::to_string(level.order_count));
}

std::string describe_spread(const OrderBook& book) {
    const auto spread = book.spread();
    if (!spread) return "spread n/a";

    std::string text = "spread " + format_price(*spread, book.instrument().price_decimals);
    if (*spread < 0) text += " (crossed)";
    if (*spread == 0) text += " (locked)";
    return text;
}

}  // namespace

std::string render_ladder(const OrderBook& book, std::size_t max_levels) {
    const int decimals = book.instrument().price_decimals;
    std::ostringstream out;
    out << book.instrument().symbol << "  orders: " << book.order_count()
        << "  bid levels: " << book.level_count(Side::Buy)
        << "  ask levels: " << book.level_count(Side::Sell) << '\n';
    write_row(out, "side", "price", "qty", "orders");

    // depth() lists the best level first; asks print in reverse so the best
    // ask ends up next to the spread line.
    const auto asks = book.depth(Side::Sell, max_levels);
    for (auto it = asks.rbegin(); it != asks.rend(); ++it) write_level(out, "ASK", *it, decimals);
    out << std::string(kTableWidth, '-') << "  " << describe_spread(book) << '\n';
    for (const DepthLevel& level : book.depth(Side::Buy, max_levels)) {
        write_level(out, "BID", level, decimals);
    }
    return out.str();
}

}  // namespace me
