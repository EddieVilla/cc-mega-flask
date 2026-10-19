#pragma once

#include <cstddef>
#include <string>

#include "me/order_book.hpp"

namespace me {

// Renders the book as a price ladder: asks on top with the best ask just above
// the spread line, bids below with the best bid just under it. Shows at most
// `max_levels` levels per side.
//
//   DEMO  orders: 4  bid levels: 2  ask levels: 2
//     side       price         qty  orders
//      ASK      100.06         500       1
//      ASK      100.05         200       1
//   --------------------------------------  spread 0.03
//      BID      100.02         300       1
//      BID      100.01         150       1
std::string render_ladder(const OrderBook& book, std::size_t max_levels = 10);

}  // namespace me
