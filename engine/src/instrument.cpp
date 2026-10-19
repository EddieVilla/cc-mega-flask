#include "me/instrument.hpp"

namespace me {

bool InstrumentRegistry::add(const Instrument& instrument) {
    const bool valid = !instrument.symbol.empty() && instrument.price_decimals >= 0 &&
                       instrument.price_decimals <= kMaxPriceDecimals && instrument.tick_size > 0 &&
                       instrument.lot_size > 0;
    if (!valid) return false;
    return instruments_.try_emplace(instrument.symbol, instrument).second;
}

const Instrument* InstrumentRegistry::find(const Symbol& symbol) const {
    const auto it = instruments_.find(symbol);
    return it == instruments_.end() ? nullptr : &it->second;
}

}  // namespace me
