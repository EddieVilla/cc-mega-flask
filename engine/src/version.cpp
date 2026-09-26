#include "me/version.hpp"

namespace me {

// ME_VERSION is set by CMake from project(VERSION ...).
std::string_view version() noexcept { return ME_VERSION; }

}  // namespace me
