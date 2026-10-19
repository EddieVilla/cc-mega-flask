#include "me/validation.hpp"

namespace me {

std::optional<RejectReason> validate(const Order& order, const Instrument& instrument) {
    if (order.symbol != instrument.symbol) return RejectReason::UnknownSymbol;
    if (order.qty <= 0) return RejectReason::InvalidQuantity;
    if (order.qty > instrument.max_order_qty) return RejectReason::QuantityTooLarge;
    if (order.qty % instrument.lot_size != 0) return RejectReason::InvalidLotSize;

    switch (order.type) {
        case OrderType::Limit:
            if (order.price <= 0) return RejectReason::InvalidPrice;
            if (order.price % instrument.tick_size != 0) return RejectReason::PriceNotOnTick;
            break;
        case OrderType::Market:
            // A market order takes whatever price the book offers.
            if (order.price != 0) return RejectReason::InvalidPrice;
            break;
    }
    return std::nullopt;
}

std::optional<RejectReason> validate(const Order& order, const InstrumentRegistry& instruments) {
    const Instrument* instrument = instruments.find(order.symbol);
    if (instrument == nullptr) return RejectReason::UnknownSymbol;
    return validate(order, *instrument);
}

}  // namespace me
