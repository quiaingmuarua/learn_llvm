#!/usr/bin/env bash
# build.sh — local dev helper for learn_llvm
# Automatically detects the installed LLVM and builds the project.
# Usage:
#   ./build.sh              — configure + build
#   ./build.sh test         — configure + build + run tests
#   ./build.sh clean        — remove build directory and rebuild
#   ./build.sh opt          — run a quick manual opt demo after build
set -euo pipefail

# ── Configuration ────────────────────────────────────────────────────────────
BUILD_DIR="${BUILD_DIR:-build}"
GENERATOR="${GENERATOR:-Ninja}"
BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"

# ── Detect LLVM ──────────────────────────────────────────────────────────────
detect_llvm() {
    # Try common versioned names first, then unversioned
    for cmd in llvm-config-17 llvm-config-16 llvm-config-15 llvm-config-14 llvm-config; do
        if command -v "$cmd" &>/dev/null; then
            echo "$cmd"
            return 0
        fi
    done
    echo "ERROR: llvm-config not found. Install LLVM (e.g. apt install llvm-17-dev)" >&2
    exit 1
}

LLVM_CONFIG=$(detect_llvm)
LLVM_CMAKE_DIR=$("$LLVM_CONFIG" --cmakedir)
LLVM_VERSION=$("$LLVM_CONFIG" --version)
echo "Using LLVM ${LLVM_VERSION} (${LLVM_CMAKE_DIR})"

# ── Detect C++ compiler ───────────────────────────────────────────────────────
# Prefer g++ (avoids libstdc++ link issues on some Ubuntu setups with clang++)
if command -v g++ &>/dev/null; then
    CC="${CC:-gcc}"; CXX="${CXX:-g++}"
elif command -v clang++ &>/dev/null; then
    CC="${CC:-clang}"; CXX="${CXX:-clang++}"
else
    echo "ERROR: No C++ compiler found" >&2; exit 1
fi
echo "Using compiler: ${CXX}"

# ── Handle commands ───────────────────────────────────────────────────────────
CMD="${1:-build}"

if [[ "$CMD" == "clean" ]]; then
    echo "Removing ${BUILD_DIR}..."
    rm -rf "${BUILD_DIR}"
    CMD="build"
fi

# ── CMake configure ───────────────────────────────────────────────────────────
if [[ ! -f "${BUILD_DIR}/build.ninja" && ! -f "${BUILD_DIR}/Makefile" ]]; then
    echo "Configuring..."
    cmake -S . -B "${BUILD_DIR}" \
        -DLLVM_DIR="${LLVM_CMAKE_DIR}" \
        -DCMAKE_C_COMPILER="${CC}" \
        -DCMAKE_CXX_COMPILER="${CXX}" \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DLEARN_LLVM_ENABLE_ONE_BACKEND=OFF \
        -G "${GENERATOR}"
fi

# ── Build ─────────────────────────────────────────────────────────────────────
echo "Building..."
cmake --build "${BUILD_DIR}" --parallel "$(nproc)"

# ── Test ──────────────────────────────────────────────────────────────────────
if [[ "$CMD" == "test" ]]; then
    echo "Running tests..."
    (cd "${BUILD_DIR}" && ctest --output-on-failure --parallel "$(nproc)")
fi

# ── Quick opt demo ────────────────────────────────────────────────────────────
if [[ "$CMD" == "opt" ]]; then
    KOTO_SO="${BUILD_DIR}/lib/Kotoamatsukami.so"
    if [[ ! -f "$KOTO_SO" ]]; then
        echo "ERROR: ${KOTO_SO} not found, build first" >&2; exit 1
    fi
    mkdir -p /tmp/koto_demo
    echo "Generating IR from example/test.c..."
    clang -O1 -S -emit-llvm ./example/test.c -o /tmp/koto_demo/test_O1.ll
    echo "Applying flatten pass..."
    opt -load-pass-plugin "${KOTO_SO}" \
        -passes=flatten \
        -S /tmp/koto_demo/test_O1.ll \
        -o /tmp/koto_demo/test_flat.ll
    echo "Done → /tmp/koto_demo/test_flat.ll"
fi

echo "✓ build.sh done"
