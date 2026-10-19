#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

#include "me/instrument.hpp"
#include "me/order.hpp"
#include "me/price_level.hpp"
#include "me/types.hpp"

namespace me {

// One price level as shown in market depth.
struct DepthLevel {
    Price price = 0;
    Qty total_qty = 0;
    std::size_t order_count = 0;

    bool operator==(const DepthLevel&) const = default;
};

// The resting orders for one instrument. Each side keeps its price levels
// sorted best first, and each level is a FIFO queue. This class only stores
// and looks up orders; matching comes in Phase 3.
class OrderBook {
public:
    explicit OrderBook(Instrument instrument);

    // The index holds iterators into this book's own containers. A copy would
    // still point into the original, so books can be moved but not copied.
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    OrderBook(OrderBook&&) = default;
    OrderBook& operator=(OrderBook&&) = default;

    const Instrument& instrument() const noexcept { return instrument_; }

    // Adds a limit order at the back of its price level without trying to
    // match it. Returns false, and changes nothing, if an order with the same
    // id is already resting. The order must already have passed validate().
    bool add_resting(Order order);

    // Removes a resting order and returns it, or std::nullopt if no order with
    // that id is resting.
    std::optional<Order> cancel(OrderId id);

    // Highest bid or lowest ask, or std::nullopt if that side is empty.
    std::optional<Price> best_price(Side side) const;
    std::optional<Price> best_bid() const { return best_price(Side::Buy); }
    std::optional<Price> best_ask() const { return best_price(Side::Sell); }

    // Best ask minus best bid, or std::nullopt if either side is empty.
    // Negative when the book is crossed, which can happen until Phase 3 adds
    // matching.
    std::optional<Price> spread() const;

    // Up to `max_levels` levels on one side, best price first.
    std::vector<DepthLevel> depth(Side side, std::size_t max_levels) const;

    // The resting order with this id, or nullptr.
    const Order* find(OrderId id) const;
    // The level at this price on this side, or nullptr if there is none.
    const PriceLevel* find_level(Side side, Price price) const;

    std::size_t order_count() const noexcept { return index_.size(); }
    std::size_t level_count(Side side) const noexcept { return levels(side).size(); }
    bool empty() const noexcept { return index_.empty(); }

private:
    // Sorts prices best first: highest first for bids, lowest first for asks.
    // Both sides then share one map type, and the best level is begin().
    struct BestPriceFirst {
        Side side;
        bool operator()(Price a, Price b) const noexcept {
            return side == Side::Buy ? a > b : a < b;
        }
    };
    using Levels = std::map<Price, PriceLevel, BestPriceFirst>;

    // Where a resting order lives, so cancel can go straight to it.
    struct Locator {
        Side side;
        Levels::iterator level;
        PriceLevel::iterator order;
    };

    Levels& levels(Side side) noexcept { return side == Side::Buy ? bids_ : asks_; }
    const Levels& levels(Side side) const noexcept { return side == Side::Buy ? bids_ : asks_; }

    Instrument instrument_;
    Levels bids_;
    Levels asks_;
    std::unordered_map<OrderId, Locator> index_;
};

}  // namespace me
