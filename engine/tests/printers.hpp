#pragma once

// Teaches GoogleTest to print engine types in failure messages. Without these,
// a failing EXPECT_EQ on an enum or struct prints raw bytes.

#include <ostream>

#include "me/order.hpp"
#include "me/order_book.hpp"
#include "me/types.hpp"
#include "me/validation.hpp"

namespace me {

inline void PrintTo(Side side, std::ostream* os) { *os << to_string(side); }
inline void PrintTo(OrderType type, std::ostream* os) { *os << to_string(type); }
inline void PrintTo(TimeInForce tif, std::ostream* os) { *os << to_string(tif); }
inline void PrintTo(RejectReason reason, std::ostream* os) { *os << to_string(reason); }
inline void PrintTo(const Order& order, std::ostream* os) { *os << to_string(order); }

inline void PrintTo(const DepthLevel& level, std::ostream* os) {
    *os << "{price " << level.price << ", qty " << level.total_qty << ", orders "
        << level.order_count << "}";
}

}  // namespace me
