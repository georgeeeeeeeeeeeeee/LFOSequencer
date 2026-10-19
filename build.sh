#!/bin/bash
# Build script for LFO Sequencer VST3
# Requirements: CMake 3.22+, C++17 compiler, JUCE framework

set -e

# ── Configuration ──────────────────────────────────────────────
# Set JUCE_PATH to your JUCE installation directory
# Download JUCE from: https://juce.com/download/
JUCE_PATH="${JUCE_PATH:-$HOME/JUCE}"

if [ ! -d "$JUCE_PATH" ]; then
    echo "ERROR: JUCE not found at $JUCE_PATH"
    echo ""
    echo "Options:"
    echo "  1. Clone JUCE:  git clone https://github.com/juce-framework/JUCE.git ~/JUCE"
    echo "  2. Set path:    export JUCE_PATH=/path/to/your/JUCE"
    echo ""
    exit 1
fi

echo "╔══════════════════════════════════════╗"
echo "║   Building LFO Sequencer VST3       ║"
echo "║   JUCE: $JUCE_PATH"
echo "╚══════════════════════════════════════╝"

# ── Build ──────────────────────────────────────────────────────
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DJUCE_PATH="$JUCE_PATH" \
    -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "✓ Build complete!"
echo ""

# ── Find output ────────────────────────────────────────────────
VST3_FILE=$(find . -name "*.vst3" -type d -o -name "*.vst3" -type f 2>/dev/null | head -1)
if [ -n "$VST3_FILE" ]; then
    echo "VST3 plugin: $VST3_FILE"
    echo ""
    echo "To install, copy to your VST3 folder:"
    echo "  macOS:   ~/Library/Audio/Plug-Ins/VST3/"
    echo "  Windows: C:\\Program Files\\Common Files\\VST3\\"
    echo "  Linux:   ~/.vst3/"
fi
