#include "me/order_book.hpp"

#include <algorithm>
#include <cassert>
#include <utility>

namespace me {

OrderBook::OrderBook(Instrument instrument)
    : instrument_(std::move(instrument)),
      bids_(BestPriceFirst{Side::Buy}),
      asks_(BestPriceFirst{Side::Sell}) {}

bool OrderBook::add_resting(Order order) {
    assert(order.symbol == instrument_.symbol);
    assert(order.type == OrderType::Limit && order.remaining_qty() > 0);
    if (index_.contains(order.id)) return false;

    const OrderId id = order.id;
    const Side side = order.side;
    const auto level = levels(side).try_emplace(order.price, order.price).first;
    const auto it = level->second.push_back(std::move(order));
    index_.emplace(id, Locator{side, level, it});
    return true;
}

std::optional<Order> OrderBook::cancel(OrderId id) {
    const auto found = index_.find(id);
    if (found == index_.end()) return std::nullopt;

    const Locator where = found->second;
    index_.erase(found);
    Order order = where.level->second.remove(where.order);
    if (where.level->second.empty()) levels(where.side).erase(where.level);
    return order;
}

std::optional<Price> OrderBook::best_price(Side side) const {
    const Levels& book_side = levels(side);
    if (book_side.empty()) return std::nullopt;
    return book_side.begin()->first;
}

std::optional<Price> OrderBook::spread() const {
    const auto bid = best_bid();
    const auto ask = best_ask();
    if (!bid || !ask) return std::nullopt;
    return *ask - *bid;
}

std::vector<DepthLevel> OrderBook::depth(Side side, std::size_t max_levels) const {
    const Levels& book_side = levels(side);
    std::vector<DepthLevel> result;
    result.reserve(std::min(max_levels, book_side.size()));
    for (const auto& [price, level] : book_side) {
        if (result.size() == max_levels) break;
        result.push_back({price, level.total_qty(), level.order_count()});
    }
    return result;
}

const Order* OrderBook::find(OrderId id) const {
    const auto found = index_.find(id);
    return found == index_.end() ? nullptr : &*found->second.order;
}

const PriceLevel* OrderBook::find_level(Side side, Price price) const {
    const Levels& book_side = levels(side);
    const auto found = book_side.find(price);
    return found == book_side.end() ? nullptr : &found->second;
}

}  // namespace me
