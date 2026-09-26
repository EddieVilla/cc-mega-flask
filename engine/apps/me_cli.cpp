#include <iostream>

#include "me/version.hpp"

int main() {
    std::cout << "matching engine v" << me::version() << '\n';
    return 0;
}
