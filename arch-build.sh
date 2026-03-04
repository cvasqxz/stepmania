#!/bin/bash
# Arch Linux build script for StepMania 3.9
# This script handles static linking for libraries not available as .so in Arch

set -e

echo "=== StepMania 3.9 - Arch Linux Build Script ==="
echo ""

# Check if we're in the right directory
if [ ! -f "configure" ]; then
    echo "Error: configure script not found. Run from StepMania root directory."
    exit 1
fi

# Check for required build tools
for tool in gcc g++ make autoconf; do
    if ! command -v $tool &> /dev/null; then
        echo "Error: $tool is not installed"
        exit 1
    fi
done

echo "[1/4] Checking dependencies..."
MISSING_LIBS=""

# Check for Lua (Arch uses lua or lua5.1)
if ! pkg-config --exists lua lua5.1 2>/dev/null; then
    MISSING_LIBS="$MISSING_LIBS lua"
fi

# Check for other audio/image libraries
for lib in libvorbis libmad libpng libjpeg-turbo; do
    if ! pkg-config --exists $lib 2>/dev/null; then
        MISSING_LIBS="$MISSING_LIBS $lib"
    fi
done

if [ -n "$MISSING_LIBS" ]; then
    echo "Missing libraries: $MISSING_LIBS"
    echo ""
    echo "Install with: sudo pacman -S lua51 sdl libvorbis libmad libpng libjpeg-turbo mesa glu"
    exit 1
fi

echo "✓ All dependencies found"
echo ""

echo "[2/4] Running configure..."
./configure

echo ""
echo "[3/4] Patching Makefile for Arch Linux static linking..."

# Backup original Makefile
cp src/Makefile src/Makefile.bak

# Apply static linking patches for libraries that don't have .so in Arch
# Audio libraries
sed -i 's/AUDIO_LIBS = .*/AUDIO_LIBS = -Wl,-Bstatic -lvorbisfile -lvorbis -logg -lmad -Wl,-Bdynamic/' src/Makefile

# Image libraries
sed -i 's/LIBS = .*/LIBS = -Wl,-Bstatic -lpng -ljpeg -lz -Wl,-Bdynamic -lm -lpthread/' src/Makefile

# Lua - try lua5.1 first, fallback to lua
if pkg-config --exists lua5.1 2>/dev/null; then
    echo "Using lua5.1"
    sed -i 's/LUA_LIBS = .*/LUA_LIBS = -Wl,-Bstatic -llua5.1 -Wl,-Bdynamic/' src/Makefile
else
    echo "Using lua"
    sed -i 's/LUA_LIBS = .*/LUA_LIBS = -Wl,-Bstatic -llua -Wl,-Bdynamic/' src/Makefile
fi

# Include paths for Lua
sed -i 's|LUA_CFLAGS = .*|LUA_CFLAGS = -I/usr/include/lua5.1 -I/usr/include|' src/Makefile

echo "✓ Makefile patched"
echo ""

echo "[4/4] Building..."
make clean
make -j$(nproc)

echo ""
echo "=== Build complete ==="
echo ""
echo "Binary location: src/stepmania"
echo ""
echo "To test the build:"
echo "  ./src/stepmania"
echo ""
echo "To install (optional):"
echo "  sudo make install"
