#pragma once

#include <string_view>

namespace me {

// Engine version string, e.g. "0.1.0".
std::string_view version() noexcept;

}  // namespace me
