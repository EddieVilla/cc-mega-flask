#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace me {

// Prices are fixed-point integers. The instrument's price_decimals says where
// the decimal point goes: with 2 decimals, $101.25 is stored as 10125. Integer
// prices compare exactly, which floating point does not. See price.hpp.
using Price = std::int64_t;
using Qty = std::int64_t;
using OrderId = std::uint64_t;
// Assigned by the engine as orders arrive. Lower means earlier, which decides
// time priority between orders at the same price.
using SeqNum = std::uint64_t;
// Nanoseconds since the Unix epoch.
using Timestamp = std::int64_t;
using Symbol = std::string;

enum class Side : std::uint8_t { Buy, Sell };

enum class OrderType : std::uint8_t { Limit, Market };

enum class TimeInForce : std::uint8_t {
    GTC,  // good till cancelled: unfilled quantity rests on the book
    IOC,  // immediate or cancel: fill what is possible now, cancel the rest
    FOK,  // fill or kill: fill the whole quantity now, or none of it
};

constexpr Side opposite(Side side) noexcept { return side == Side::Buy ? Side::Sell : Side::Buy; }

constexpr std::string_view to_string(Side side) noexcept {
    switch (side) {
        case Side::Buy:
            return "BUY";
        case Side::Sell:
            return "SELL";
    }
    return "?";
}

constexpr std::string_view to_string(OrderType type) noexcept {
    switch (type) {
        case OrderType::Limit:
            return "LIMIT";
        case OrderType::Market:
            return "MARKET";
    }
    return "?";
}

constexpr std::string_view to_string(TimeInForce tif) noexcept {
    switch (tif) {
        case TimeInForce::GTC:
            return "GTC";
        case TimeInForce::IOC:
            return "IOC";
        case TimeInForce::FOK:
            return "FOK";
    }
    return "?";
}

}  // namespace me
