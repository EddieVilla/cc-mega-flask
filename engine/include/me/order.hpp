#pragma once

#include <string>

#include "me/price.hpp"
#include "me/types.hpp"

namespace me {

struct Order {
    OrderId id = 0;
    Symbol symbol;
    Side side = Side::Buy;
    OrderType type = OrderType::Limit;
    TimeInForce tif = TimeInForce::GTC;
    Price price = 0;          // limit price; 0 for market orders
    Qty qty = 0;              // original quantity
    Qty filled_qty = 0;       // quantity executed so far
    SeqNum seq = 0;           // arrival order, assigned by the engine
    Timestamp timestamp = 0;  // when the engine accepted the order

    Qty remaining_qty() const noexcept { return qty - filled_qty; }
    bool is_filled() const noexcept { return filled_qty >= qty; }

    bool operator==(const Order&) const = default;
};

// One-line description for logs and the CLI, such as
// "#7 AAPL BUY 100 @ 189.50 GTC" or "#8 AAPL SELL 50 @ MKT IOC (filled 20)".
std::string to_string(const Order& order, int price_decimals = kDefaultPriceDecimals);

}  // namespace me
