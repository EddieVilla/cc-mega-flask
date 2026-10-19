// me_cli: a small shell for experimenting with one order book.
//
// Phase 2: orders rest on the book without matching, so a buy priced at or
// above the best ask leaves the book crossed. Matching comes in Phase 3.
//
// Usage:
//   me_cli            type commands at a prompt
//   me_cli <script>   run the commands in a file, echoing each one

#include <cctype>
#include <charconv>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "me/instrument.hpp"
#include "me/ladder.hpp"
#include "me/order.hpp"
#include "me/order_book.hpp"
#include "me/price.hpp"
#include "me/validation.hpp"
#include "me/version.hpp"

namespace {

constexpr std::string_view kHelp =
    "Commands:\n"
    "  ADD BUY|SELL <qty> @ <price>   rest a limit order on the book (no matching yet)\n"
    "  CANCEL <id>                    cancel a resting order\n"
    "  BOOK [levels]                  show the book, 10 levels per side by default\n"
    "  HELP                           show this help\n"
    "  QUIT                           exit\n";

std::vector<std::string> split_words(std::string_view line) {
    std::istringstream stream{std::string(line)};
    std::vector<std::string> words;
    for (std::string word; stream >> word;) words.push_back(word);
    return words;
}

std::string to_upper(std::string text) {
    for (char& c : text) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return text;
}

// Parses the whole of `text` as an integer, or returns std::nullopt.
template <typename Int>
std::optional<Int> parse_int(std::string_view text) {
    Int value{};
    const char* end = text.data() + text.size();
    const auto [stop, error] = std::from_chars(text.data(), end, value);
    if (error != std::errc{} || stop != end) return std::nullopt;
    return value;
}

std::optional<me::Side> parse_side(const std::string& text) {
    const std::string word = to_upper(text);
    if (word == "BUY") return me::Side::Buy;
    if (word == "SELL") return me::Side::Sell;
    return std::nullopt;
}

class Shell {
public:
    explicit Shell(me::Instrument instrument) : book_(std::move(instrument)) {}

    // Runs one command line and writes the response to `out`. Returns false
    // once the user asks to quit.
    bool execute(std::string_view line, std::ostream& out) {
        const std::vector<std::string> words = split_words(line);
        if (words.empty() || words[0].starts_with('#')) return true;

        const std::string command = to_upper(words[0]);
        if (command == "ADD") {
            add(words, out);
        } else if (command == "CANCEL") {
            cancel(words, out);
        } else if (command == "BOOK") {
            show_book(words, out);
        } else if (command == "HELP") {
            out << kHelp;
        } else if (command == "QUIT" || command == "EXIT") {
            return false;
        } else {
            out << "ERROR unknown command \"" << words[0] << "\" (try HELP)\n";
        }
        return true;
    }

private:
    int decimals() const { return book_.instrument().price_decimals; }

    // ADD BUY|SELL <qty> @ <price>, where the "@" is optional.
    void add(const std::vector<std::string>& words, std::ostream& out) {
        const bool with_at = words.size() == 5 && words[3] == "@";
        if (words.size() != 4 && !with_at) {
            out << "ERROR usage: ADD BUY|SELL <qty> @ <price>\n";
            return;
        }
        const auto side = parse_side(words[1]);
        if (!side) {
            out << "ERROR side must be BUY or SELL, not \"" << words[1] << "\"\n";
            return;
        }
        const auto qty = parse_int<me::Qty>(words[2]);
        if (!qty) {
            out << "ERROR invalid quantity \"" << words[2] << "\"\n";
            return;
        }
        const auto price = me::parse_price(words.back(), decimals());
        if (!price) {
            out << "ERROR invalid price \"" << words.back() << "\"\n";
            return;
        }

        // Until the engine exists, the shell hands out ids. One counter also
        // serves as the sequence number.
        me::Order order{.id = next_id_,
                        .symbol = book_.instrument().symbol,
                        .side = *side,
                        .price = *price,
                        .qty = *qty,
                        .seq = next_id_};
        if (const auto reject = me::validate(order, book_.instrument())) {
            out << "REJECT " << me::to_string(*reject) << '\n';
            return;
        }
        ++next_id_;
        out << "ACK " << me::to_string(order, decimals()) << '\n';
        book_.add_resting(std::move(order));
    }

    // CANCEL <id>
    void cancel(const std::vector<std::string>& words, std::ostream& out) {
        const auto id = words.size() == 2 ? parse_int<me::OrderId>(words[1]) : std::nullopt;
        if (!id) {
            out << "ERROR usage: CANCEL <id>\n";
            return;
        }
        if (const auto order = book_.cancel(*id)) {
            out << "CANCELLED " << me::to_string(*order, decimals()) << '\n';
        } else {
            out << "ERROR no resting order with id " << *id << '\n';
        }
    }

    // BOOK [levels]
    void show_book(const std::vector<std::string>& words, std::ostream& out) {
        std::optional<std::size_t> levels = 10;
        if (words.size() == 2) levels = parse_int<std::size_t>(words[1]);
        if (words.size() > 2 || !levels) {
            out << "ERROR usage: BOOK [levels]\n";
            return;
        }
        out << me::render_ladder(book_, *levels);
    }

    me::OrderBook book_;
    me::OrderId next_id_ = 1;
};

}  // namespace

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "usage: " << argv[0] << " [script]\n";
        return 2;
    }
    std::ifstream script;
    if (argc == 2) {
        script.open(argv[1]);
        if (!script) {
            std::cerr << "error: cannot open " << argv[1] << '\n';
            return 1;
        }
    }
    const bool interactive = argc == 1;
    std::istream& in = interactive ? std::cin : script;

    if (interactive) {
        std::cout << "matching engine v" << me::version() << ", type HELP for commands\n";
    }

    // Prices in cents, any whole quantity.
    Shell shell{{.symbol = "DEMO"}};
    std::string line;
    while (true) {
        if (interactive) std::cout << "> " << std::flush;
        if (!std::getline(in, line)) {
            if (interactive) std::cout << '\n';  // end the prompt line after Ctrl-D
            break;
        }
        if (!line.empty() && line.back() == '\r') line.pop_back();  // Windows line endings
        if (!interactive) std::cout << (line.empty() ? "" : "> ") << line << '\n';
        if (!shell.execute(line, std::cout)) break;
    }
    return 0;
}
