#include "me/order.hpp"

namespace me {

std::string to_string(const Order& order, int price_decimals) {
    std::string out = "#" + std::to_string(order.id) + " " + order.symbol + " ";
    out += to_string(order.side);
    out += " " + std::to_string(order.qty) + " @ ";
    out += order.type == OrderType::Market ? "MKT" : format_price(order.price, price_decimals);
    out += " ";
    out += to_string(order.tif);
    if (order.filled_qty > 0) out += " (filled " + std::to_string(order.filled_qty) + ")";
    return out;
}

}  // namespace me
