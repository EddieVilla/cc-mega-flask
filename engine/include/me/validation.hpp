#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "me/instrument.hpp"
#include "me/order.hpp"

namespace me {

enum class RejectReason : std::uint8_t {
    UnknownSymbol,
    InvalidQuantity,   // zero or negative
    QuantityTooLarge,  // above the instrument's maximum order size
    InvalidLotSize,    // not a multiple of the instrument's lot size
    InvalidPrice,      // limit price <= 0, or a market order with a price
    PriceNotOnTick,    // limit price not a multiple of the tick size
};

constexpr std::string_view to_string(RejectReason reason) noexcept {
    switch (reason) {
        case RejectReason::UnknownSymbol:
            return "unknown symbol";
        case RejectReason::InvalidQuantity:
            return "invalid quantity";
        case RejectReason::QuantityTooLarge:
            return "quantity above maximum";
        case RejectReason::InvalidLotSize:
            return "quantity not a multiple of lot size";
        case RejectReason::InvalidPrice:
            return "invalid price";
        case RejectReason::PriceNotOnTick:
            return "price not on tick";
    }
    return "?";
}

// Checks a new order against the rules of `instrument`. Returns the first
// problem found, or std::nullopt if the order is acceptable.
std::optional<RejectReason> validate(const Order& order, const Instrument& instrument);

// Same, after looking up the order's symbol. Unregistered symbols are rejected.
std::optional<RejectReason> validate(const Order& order, const InstrumentRegistry& instruments);

}  // namespace me
