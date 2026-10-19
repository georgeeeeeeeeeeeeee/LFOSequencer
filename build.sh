#!/bin/bash
# Build script for LFO Sequencer VST3
# Requirements: CMake 3.22+, C++17 compiler, git (to download JUCE)

set -e

# ── Configuration ──────────────────────────────────────────────
# JUCE is downloaded automatically at the version pinned in CMakeLists.txt.
# To use a local copy instead: export JUCE_PATH=/path/to/your/JUCE
JUCE_ARGS=()
if [ -n "$JUCE_PATH" ]; then
    if [ ! -d "$JUCE_PATH" ]; then
        echo "ERROR: JUCE_PATH is set but $JUCE_PATH doesn't exist"
        exit 1
    fi
    JUCE_ARGS=(-DJUCE_PATH="$JUCE_PATH")
fi

echo "╔══════════════════════════════════════╗"
echo "║   Building LFO Sequencer VST3       ║"
echo "║   JUCE: ${JUCE_PATH:-pinned download}"
echo "╚══════════════════════════════════════╝"

# ── Build ──────────────────────────────────────────────────────
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    "${JUCE_ARGS[@]}" \
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
fi
