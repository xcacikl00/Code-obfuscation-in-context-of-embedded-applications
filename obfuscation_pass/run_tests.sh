#!/bin/bash
set -e
source "$HOME/.virtualenvs/angr/bin/activate"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD="$SCRIPT_DIR/llvm-test-suite/build"
PLUGIN="$SCRIPT_DIR/build/libObfuscator.so"
STATE_FILE="$BUILD/.last_cmake_state"

export OBF_FLATTEN=${OBF_FLATTEN:-1}
export OBF_BOGUS=${OBF_BOGUS:-1}
export OBF_SUBST=${OBF_SUBST:-1}
export OBF_DEBUG=1

# build the plugin first
cmake -S "$SCRIPT_DIR" -B "$SCRIPT_DIR/build"
make -C "$SCRIPT_DIR/build" -j$(nproc)

CURRENT_STATE="OBF_FLATTEN=${OBF_FLATTEN} OBF_BOGUS=${OBF_BOGUS} OBF_SUBST=${OBF_SUBST}"
LAST_STATE=$(cat "$STATE_FILE" 2>/dev/null || echo "")

if [ "$CURRENT_STATE" != "$LAST_STATE" ] || [ ! -f "$BUILD/CMakeCache.txt" ]; then
    echo "CMake state changed or missing, reconfiguring..."
    rm -rf "$BUILD"/*
    cmake -S "$SCRIPT_DIR/llvm-test-suite" \
          -B "$BUILD" \
          -DCMAKE_C_COMPILER=clang \
          -DCMAKE_CXX_COMPILER=clang++ \
          -DCMAKE_C_FLAGS="-fpass-plugin=${PLUGIN}" \
          -DCMAKE_CXX_FLAGS="-fpass-plugin=${PLUGIN}"
    echo "$CURRENT_STATE" > "$STATE_FILE"
else
    echo "CMake state unchanged, skipping reconfigure..."
fi

make -C "$BUILD/SingleSource/UnitTests" -B -j$(nproc) 

lit -v "$BUILD/SingleSource/UnitTests" | tee -a  log.txt