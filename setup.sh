#!/bin/bash
# setup.sh — project-mark macOS: install dependencies and build
# Run this once from the project root: bash setup.sh

set -e
cd "$(dirname "$0")"

echo "=== Step 1: Install system dependencies ==="
# Check Homebrew
if ! command -v brew &>/dev/null; then
    echo "ERROR: Homebrew not found. Install it first:"
    echo '  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/homebrew/install/HEAD/install.sh)"'
    exit 1
fi

brew install cmake cryptopp zstd

echo ""
echo "=== Step 2: Download imgui Metal backends ==="
# The Windows repo has imgui_impl_dx11 / imgui_impl_win32.
# We need the Metal + OSX backends from the official imgui repo.
BACKENDS_DIR="Module/Dependencies/Dependencies/imgui/backends"
mkdir -p "$BACKENDS_DIR"

IMGUI_RAW="https://raw.githubusercontent.com/ocornut/imgui/master/backends"

# Only download if not already present
if [ ! -f "$BACKENDS_DIR/imgui_impl_metal.mm" ]; then
    echo "Downloading imgui_impl_metal.mm..."
    curl -fsSL "$IMGUI_RAW/imgui_impl_metal.h"  -o "$BACKENDS_DIR/imgui_impl_metal.h"
    curl -fsSL "$IMGUI_RAW/imgui_impl_metal.mm" -o "$BACKENDS_DIR/imgui_impl_metal.mm"
fi

if [ ! -f "$BACKENDS_DIR/imgui_impl_osx.mm" ]; then
    echo "Downloading imgui_impl_osx.mm..."
    curl -fsSL "$IMGUI_RAW/imgui_impl_osx.h"  -o "$BACKENDS_DIR/imgui_impl_osx.h"
    curl -fsSL "$IMGUI_RAW/imgui_impl_osx.mm" -o "$BACKENDS_DIR/imgui_impl_osx.mm"
fi

echo "imgui Metal backends ready."

echo ""
echo "=== Step 3: Uncomment Metal backends in CMakeLists.txt ==="
# Uncomment the two Metal backend lines in CMakeLists.txt
sed -i '' \
    's|# \${DEPS}/imgui/backends/imgui_impl_metal.mm|\${DEPS}/imgui/backends/imgui_impl_metal.mm|g' \
    CMakeLists.txt
sed -i '' \
    's|# \${DEPS}/imgui/backends/imgui_impl_osx.mm|\${DEPS}/imgui/backends/imgui_impl_osx.mm|g' \
    CMakeLists.txt
echo "CMakeLists.txt updated."

echo ""
echo "=== Step 4: Configure with CMake ==="
# Detect architecture
ARCH=$(uname -m)
if [ "$ARCH" = "arm64" ]; then
    CMAKE_ARCH="arm64"
else
    CMAKE_ARCH="x86_64"
fi
echo "Building for: $CMAKE_ARCH"

cmake -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_ARCHITECTURES="$CMAKE_ARCH"

echo ""
echo "=== Step 5: Build ==="
cmake --build build -j$(sysctl -n hw.logicalcpu)

echo ""
echo "=== Done ==="
echo "Output: build/libCobalt.dylib"
echo ""
echo "To verify:"
echo "  file build/libCobalt.dylib"
echo "  nm -g build/libCobalt.dylib | grep -v 'U '"
