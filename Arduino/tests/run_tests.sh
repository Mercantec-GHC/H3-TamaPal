#!/bin/bash
# WHAT: Script to compile and run TamaPal C++ unit tests on Mac/Linux
# WHY:  Enables instant test-driven development without touching physical hardware

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/../TamaPal"

echo "Compiling TamaPal unit tests..."
clang++ -std=c++17 -Wall -Wextra -I"$PROJECT_DIR" \
    "$SCRIPT_DIR/test_main.cpp" \
    "$PROJECT_DIR/menu_logic.cpp" \
    "$PROJECT_DIR/tilt_logic.cpp" \
    "$PROJECT_DIR/pet_logic.cpp" \
    -o "$SCRIPT_DIR/test_runner"

echo "Running tests..."
"$SCRIPT_DIR/test_runner"

