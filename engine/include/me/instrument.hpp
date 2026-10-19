#pragma once

#include <cstddef>
#include <unordered_map>

#include "me/price.hpp"
#include "me/types.hpp"

namespace me {

// Trading rules for one symbol.
struct Instrument {
    Symbol symbol;
    int price_decimals = kDefaultPriceDecimals;  // 2 means prices are stored in cents
    Price tick_size = 1;  // minimum price step, in price units (1 = 0.01 with 2 decimals)
    Qty lot_size = 1;     // order quantities must be a multiple of this
};

// The symbols the engine trades and their rules.
class InstrumentRegistry {
public:
    // Adds an instrument. Returns false, and changes nothing, if the symbol is
    // empty or already registered, or a setting is out of range.
    bool add(const Instrument& instrument);

    // The instrument for `symbol`, or nullptr if it isn't registered. The
    // pointer stays valid as more instruments are added.
    const Instrument* find(const Symbol& symbol) const;

    std::size_t size() const noexcept { return instruments_.size(); }

private:
    std::unordered_map<Symbol, Instrument> instruments_;
};

}  // namespace me
