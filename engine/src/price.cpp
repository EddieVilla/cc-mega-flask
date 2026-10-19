#include "me/price.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace me {
namespace {

constexpr std::uint64_t pow10(int exponent) {
    std::uint64_t result = 1;
    for (int i = 0; i < exponent; ++i) result *= 10;
    return result;
}

constexpr bool is_digit(char c) { return c >= '0' && c <= '9'; }

}  // namespace

std::optional<Price> parse_price(std::string_view text, int decimals) {
    assert(decimals >= 0 && decimals <= kMaxPriceDecimals);
    const auto places = static_cast<std::size_t>(decimals);

    const bool negative = !text.empty() && text.front() == '-';
    if (negative) text.remove_prefix(1);

    // Split "123.45" into "123" and "45".
    const std::size_t dot = text.find('.');
    const bool has_dot = dot != std::string_view::npos;
    const std::string_view whole = text.substr(0, dot);
    std::string_view fraction = has_dot ? text.substr(dot + 1) : std::string_view{};
    if (whole.empty() || (has_dot && fraction.empty())) return std::nullopt;

    // Digits beyond the allowed precision must all be zeros.
    if (fraction.size() > places) {
        if (fraction.find_first_not_of('0', places) != std::string_view::npos) return std::nullopt;
        fraction = fraction.substr(0, places);
    }

    // Shift digits in one at a time, refusing any that would overflow.
    constexpr Price kMax = std::numeric_limits<Price>::max();
    Price value = 0;
    auto append_digit = [&value](char c) {
        if (!is_digit(c)) return false;
        const int digit = c - '0';
        if (value > (kMax - digit) / 10) return false;
        value = value * 10 + digit;
        return true;
    };
    for (const char c : whole) {
        if (!append_digit(c)) return std::nullopt;
    }
    for (const char c : fraction) {
        if (!append_digit(c)) return std::nullopt;
    }
    // Pad short fractions: "101.5" becomes 10150 with 2 decimals.
    for (std::size_t i = fraction.size(); i < places; ++i) {
        if (!append_digit('0')) return std::nullopt;
    }
    return negative ? -value : value;
}

std::string format_price(Price price, int decimals) {
    assert(decimals >= 0 && decimals <= kMaxPriceDecimals);
    const std::uint64_t scale = pow10(decimals);

    // Work with the magnitude as unsigned so the most negative price, which
    // has no positive int64 counterpart, doesn't overflow.
    const std::uint64_t magnitude = price < 0 ? std::uint64_t{0} - static_cast<std::uint64_t>(price)
                                              : static_cast<std::uint64_t>(price);

    std::string out = price < 0 ? "-" : "";
    out += std::to_string(magnitude / scale);
    if (decimals > 0) {
        const std::string fraction = std::to_string(magnitude % scale);
        out += '.';
        out.append(static_cast<std::size_t>(decimals) - fraction.size(), '0');
        out += fraction;
    }
    return out;
}

}  // namespace me
