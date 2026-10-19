// Phase 1 checkpoint: build a few orders and show how validation treats them.

#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "me/instrument.hpp"
#include "me/order.hpp"
#include "me/price.hpp"
#include "me/validation.hpp"
#include "me/version.hpp"

int main() {
    using namespace me;

    std::cout << "matching engine v" << version() << "\n\n";

    std::cout << "Prices are stored as integers (2 decimals):\n";
    for (const std::string_view text : {"101.25", "101.5", "101.255", "abc"}) {
        const auto price = parse_price(text);
        std::cout << "  parse_price(\"" << text
                  << "\") = " << (price ? std::to_string(*price) : "invalid") << '\n';
    }

    // DEMO trades in cents with a 0.05 tick and round lots of 10.
    InstrumentRegistry instruments;
    instruments.add({.symbol = "DEMO", .tick_size = 5, .lot_size = 10});

    const std::vector<Order> orders = {
        {.id = 1, .symbol = "DEMO", .side = Side::Buy, .price = 10025, .qty = 100},
        {.id = 2,
         .symbol = "DEMO",
         .side = Side::Sell,
         .type = OrderType::Market,
         .tif = TimeInForce::IOC,
         .qty = 50},
        {.id = 3, .symbol = "DEMO", .side = Side::Buy, .price = 10027, .qty = 100},
        {.id = 4, .symbol = "DEMO", .side = Side::Sell, .price = 10050, .qty = 0},
        {.id = 5, .symbol = "DEMO", .side = Side::Sell, .price = 10050, .qty = 15},
        {.id = 6, .symbol = "DEMO", .side = Side::Sell, .price = 0, .qty = 10},
        {.id = 7, .symbol = "NOPE", .side = Side::Buy, .price = 10000, .qty = 10},
    };

    std::cout << "\nValidating orders for DEMO (tick 0.05, lot 10):\n";
    for (const Order& order : orders) {
        const auto reject = validate(order, instruments);
        std::cout << "  " << std::left << std::setw(32) << to_string(order)
                  << (reject ? "REJECT " + std::string(to_string(*reject)) : "OK") << '\n';
    }
    return 0;
}
