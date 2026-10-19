#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "me/types.hpp"

namespace me {

inline constexpr int kDefaultPriceDecimals = 2;
inline constexpr int kMaxPriceDecimals = 18;

// Converts a decimal string to a fixed-point price with `decimals` places:
// parse_price("101.25") == 10125. Accepts an optional leading '-', then
// digits, then optionally '.' and more digits. Returns std::nullopt if the
// text is malformed, out of range, or more precise than `decimals` allows.
// Extra trailing zeros are fine: "101.250" parses, "101.255" does not.
// `decimals` must be between 0 and kMaxPriceDecimals.
std::optional<Price> parse_price(std::string_view text, int decimals = kDefaultPriceDecimals);

// Formats a fixed-point price with `decimals` places:
// format_price(10125) == "101.25". The inverse of parse_price.
std::string format_price(Price price, int decimals = kDefaultPriceDecimals);

}  // namespace me
