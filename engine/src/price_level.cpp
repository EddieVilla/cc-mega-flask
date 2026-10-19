#include "me/price_level.hpp"

#include <utility>

namespace me {

PriceLevel::iterator PriceLevel::push_back(Order order) {
    total_qty_ += order.remaining_qty();
    return orders_.insert(orders_.end(), std::move(order));
}

Order PriceLevel::remove(iterator it) {
    total_qty_ -= it->remaining_qty();
    Order order = std::move(*it);
    orders_.erase(it);
    return order;
}

}  // namespace me
