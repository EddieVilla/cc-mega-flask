#include "me/instrument.hpp"

#include <utility>

namespace me {

bool InstrumentRegistry::add(Instrument instrument) {
    const bool valid = !instrument.symbol.empty() && instrument.price_decimals >= 0 &&
                       instrument.price_decimals <= kMaxPriceDecimals && instrument.tick_size > 0 &&
                       instrument.lot_size > 0;
    if (!valid) return false;

    Symbol key = instrument.symbol;
    return instruments_.try_emplace(std::move(key), std::move(instrument)).second;
}

const Instrument* InstrumentRegistry::find(const Symbol& symbol) const {
    const auto it = instruments_.find(symbol);
    return it == instruments_.end() ? nullptr : &it->second;
}

}  // namespace me
