#pragma once

#include <cstddef>
#include <list>

#include "me/order.hpp"
#include "me/types.hpp"

namespace me {

// The resting orders at one price on one side of the book, in time priority:
// the front order arrived first and will fill first.
class PriceLevel {
public:
    // std::list keeps each order at a fixed address, so an iterator to it
    // stays valid while other orders come and go. The book relies on that to
    // cancel an order without searching for it.
    using Orders = std::list<Order>;
    using iterator = Orders::iterator;
    using const_iterator = Orders::const_iterator;

    explicit PriceLevel(Price price) : price_(price) {}

    Price price() const noexcept { return price_; }
    // Sum of the remaining quantity of every order at this level.
    Qty total_qty() const noexcept { return total_qty_; }
    std::size_t order_count() const noexcept { return orders_.size(); }
    bool empty() const noexcept { return orders_.empty(); }

    const Order& front() const { return orders_.front(); }
    const_iterator begin() const noexcept { return orders_.begin(); }
    const_iterator end() const noexcept { return orders_.end(); }

    // Adds an order at the back of the queue and returns an iterator to it.
    iterator push_back(Order order);

    // Takes the order at `it` out of the queue and returns it.
    Order remove(iterator it);

private:
    Price price_;
    Qty total_qty_ = 0;
    Orders orders_;
};

}  // namespace me
