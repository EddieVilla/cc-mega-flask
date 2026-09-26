#!/usr/bin/env bash
# Configure, build, and run the tests for a preset (default: debug).
# Usage: scripts/test.sh [debug|release]
set -euo pipefail

preset="${1:-debug}"
cd "$(dirname "$0")/.."

cmake --preset "$preset"
cmake --build --preset "$preset" -j
ctest --preset "$preset"
