#!/usr/bin/env bash
# build.sh — local dev helper for learn_llvm
# Automatically detects the installed LLVM and builds the project.
# On macOS/Homebrew it prefers llvm@17, which matches this repository's tested setup.
# Usage:
#   ./build.sh              — configure + build
#   ./build.sh test         — configure + build + run tests
#   ./build.sh clean        — remove build directory and rebuild
#   ./build.sh opt          — run a quick manual opt demo after build
set -euo pipefail

# ── Configuration ────────────────────────────────────────────────────────────
BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"

# ── Detect LLVM ──────────────────────────────────────────────────────────────
detect_llvm() {
    local candidates=(
        llvm-config-17
        llvm-config-16
        llvm-config-15
        llvm-config-14
        llvm-config
    )

    # Try common command names first.
    for cmd in "${candidates[@]}"; do
        if command -v "$cmd" >/dev/null 2>&1; then
            command -v "$cmd"
            return 0
        fi
    done

    # Homebrew LLVM is often installed but not added to PATH on macOS.
    if command -v brew >/dev/null 2>&1; then
        local formula brew_prefix
        for formula in llvm@17 llvm@16 llvm@15 llvm@14 llvm; do
            brew_prefix="$(brew --prefix "${formula}" 2>/dev/null || true)"
            if [[ -n "${brew_prefix}" && -x "${brew_prefix}/bin/llvm-config" ]]; then
                echo "${brew_prefix}/bin/llvm-config"
                return 0
            fi
        done
    fi

    echo "ERROR: llvm-config not found." >&2
    echo "Install LLVM and ensure llvm-config is available." >&2
    echo "Examples:" >&2
    echo "  macOS (Homebrew): xcode-select --install && brew install cmake ninja llvm@17" >&2
    echo "  Ubuntu/Debian: apt install llvm-17-dev" >&2
    exit 1
}

detect_parallelism() {
    if command -v nproc >/dev/null 2>&1; then
        nproc
        return 0
    fi

    if command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.ncpu
        return 0
    fi

    if command -v getconf >/dev/null 2>&1; then
        getconf _NPROCESSORS_ONLN
        return 0
    fi

    echo 4
}

detect_generator() {
    if command -v ninja >/dev/null 2>&1 || command -v ninja-build >/dev/null 2>&1; then
        echo "Ninja"
        return 0
    fi

    echo "Unix Makefiles"
}

detect_tool_from_llvm_bin() {
    local tool_name="$1"
    local tool_path="${LLVM_BIN_DIR}/${tool_name}"
    if [[ -x "${tool_path}" ]]; then
        echo "${tool_path}"
        return 0
    fi

    if command -v "${tool_name}" >/dev/null 2>&1; then
        command -v "${tool_name}"
        return 0
    fi

    return 1
}

detect_plugin_artifact() {
    local plugin_name="$1"
    local candidates=(
        "${BUILD_DIR}/lib/${plugin_name}.so"
        "${BUILD_DIR}/lib/${plugin_name}.dylib"
    )

    for candidate in "${candidates[@]}"; do
        if [[ -f "${candidate}" ]]; then
            echo "${candidate}"
            return 0
        fi
    done

    return 1
}

detect_compilers() {
    # Respect explicit user overrides if both are provided.
    if [[ -n "${CC:-}" || -n "${CXX:-}" ]]; then
        if [[ -z "${CC:-}" || -z "${CXX:-}" ]]; then
            echo "ERROR: set both CC and CXX together, or leave both unset." >&2
            exit 1
        fi
        if ! command -v "${CC}" >/dev/null 2>&1; then
            echo "ERROR: CC not found: ${CC}" >&2
            exit 1
        fi
        if ! command -v "${CXX}" >/dev/null 2>&1; then
            echo "ERROR: CXX not found: ${CXX}" >&2
            exit 1
        fi
        echo "${CC};${CXX}"
        return 0
    fi

    if [[ "$(uname -s)" == "Darwin" ]]; then
        local llvm_clang llvm_clangxx xcrun_clang xcrun_clangxx
        llvm_clang="$(detect_tool_from_llvm_bin clang || true)"
        llvm_clangxx="$(detect_tool_from_llvm_bin clang++ || true)"
        if [[ -n "${llvm_clang}" && -n "${llvm_clangxx}" ]]; then
            echo "${llvm_clang};${llvm_clangxx}"
            return 0
        fi

        xcrun_clang="$(xcrun --find clang 2>/dev/null || true)"
        xcrun_clangxx="$(xcrun --find clang++ 2>/dev/null || true)"
        if [[ -n "${xcrun_clang}" && -n "${xcrun_clangxx}" ]]; then
            echo "${xcrun_clang};${xcrun_clangxx}"
            return 0
        fi
    fi

    # On Linux-like environments, prefer GNU compilers when available.
    if command -v gcc >/dev/null 2>&1 && command -v g++ >/dev/null 2>&1; then
        echo "$(command -v gcc);$(command -v g++)"
        return 0
    fi

    if command -v clang >/dev/null 2>&1 && command -v clang++ >/dev/null 2>&1; then
        echo "$(command -v clang);$(command -v clang++)"
        return 0
    fi

    echo "ERROR: no suitable C/C++ compiler pair found." >&2
    exit 1
}

LLVM_CONFIG=$(detect_llvm)
LLVM_BIN_DIR="$(cd "$(dirname "${LLVM_CONFIG}")" && pwd)"
LLVM_CMAKE_DIR=$("$LLVM_CONFIG" --cmakedir)
LLVM_VERSION=$("$LLVM_CONFIG" --version)
LLVM_MAJOR="${LLVM_VERSION%%.*}"
GENERATOR="${GENERATOR:-$(detect_generator)}"
echo "Using LLVM ${LLVM_VERSION} (${LLVM_CMAKE_DIR})"
echo "Using LLVM tools from ${LLVM_BIN_DIR}"
echo "Using generator: ${GENERATOR}"
PARALLELISM="${PARALLELISM:-$(detect_parallelism)}"

if [[ "${LLVM_MAJOR}" -lt 14 || "${LLVM_MAJOR}" -gt 17 ]]; then
    echo "WARNING: This repository is tested with LLVM 14-17; LLVM ${LLVM_VERSION} may not build cleanly." >&2
fi

# ── Detect C++ compiler ───────────────────────────────────────────────────────
IFS=';' read -r CC CXX <<< "$(detect_compilers)"
echo "Using C compiler: ${CC}"
echo "Using C++ compiler: ${CXX}"

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
cmake --build "${BUILD_DIR}" --parallel "${PARALLELISM}"

# ── Test ──────────────────────────────────────────────────────────────────────
if [[ "$CMD" == "test" ]]; then
    echo "Running tests..."
    (cd "${BUILD_DIR}" && ctest --output-on-failure --parallel "${PARALLELISM}")
fi

# ── Quick opt demo ────────────────────────────────────────────────────────────
if [[ "$CMD" == "opt" ]]; then
    KOTO_PLUGIN="$(detect_plugin_artifact Kotoamatsukami || true)"
    if [[ -z "${KOTO_PLUGIN}" ]]; then
        echo "ERROR: Kotoamatsukami plugin not found under ${BUILD_DIR}/lib (expected .so or .dylib), build first" >&2
        exit 1
    fi
    mkdir -p /tmp/koto_demo
    CLANG_BIN="$(detect_tool_from_llvm_bin clang || command -v clang)"
    OPT_BIN="$(detect_tool_from_llvm_bin opt)"
    echo "Generating IR from example/test.c..."
    "${CLANG_BIN}" -O1 -S -emit-llvm ./example/test.c -o /tmp/koto_demo/test_O1.ll
    echo "Applying flatten pass..."
    "${OPT_BIN}" -load-pass-plugin "${KOTO_PLUGIN}" \
        -passes=flatten \
        -S /tmp/koto_demo/test_O1.ll \
        -o /tmp/koto_demo/test_flat.ll
    echo "Done → /tmp/koto_demo/test_flat.ll"
fi

echo "✓ build.sh done"
