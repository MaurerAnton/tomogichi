#!/bin/sh
# Build Tomogichi v0.5.0 on any Linux system (arm64 or amd64)
# Run this from the tomogichi repo root
set -e

echo "=== Building Tomogichi v0.5.0 CLI ==="
g++ -std=c++17 -O2 -o tomogichi \
    src/main.cpp \
    src/core/engine.cpp \
    src/data/storage.cpp \
    -I src -I src/core -I src/data

echo "=== CLI binary: $(pwd)/tomogichi ==="
./tomogichi --version

echo ""
echo "=== For Qt GUI (optional, needs Qt6 + Kirigami): ==="
echo "  cmake -S qt -B qt/build && cmake --build qt/build"
echo ""
echo "=== To run: ==="
echo "  ./tomogichi                    # CLI interactive mode"
echo "  ./qt/build/tomogichi-qt        # Qt GUI (if built)"
