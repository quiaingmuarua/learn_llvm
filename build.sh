#!/usr/bin/env bash
# build.sh — cross-platform local dev helper for learn_llvm
# Supports macOS, Linux, and WSL by auto-detecting LLVM, compilers, and a CMake generator.
# Usage:
#   ./build.sh              configure + build
#   ./build.sh test         configure + build + run tests
#   ./build.sh clean        remove build directory and rebuild
#   ./build.sh opt          configure + build + run a quick opt demo
# Environment overrides:
#   BUILD_DIR, CMAKE_BUILD_TYPE, GENERATOR, PARALLELISM, LLVM_CONFIG, LLVM_DIR, CC, CXX,
#   LEARN_LLVM_ENABLE_ONE_BACKEND, BUILD_TESTING
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"
ENABLE_ONE_BACKEND="${LEARN_LLVM_ENABLE_ONE_BACKEND:-OFF}"
BUILD_TESTING_OPTION="${BUILD_TESTING:-ON}"
HOST_OS="$(uname -s)"
KERNEL_RELEASE="$(uname -r 2>/dev/null || true)"

have_cmd() {
    command -v "$1" >/dev/null 2>&1
}

die() {
    echo "ERROR: $*" >&2
    exit 1
}

warn() {
    echo "WARNING: $*" >&2
}

lookup_cmd() {
    local cmd
    for cmd in "$@"; do
        if have_cmd "${cmd}"; then
            command -v "${cmd}"
            return 0
        fi
    done
    return 1
}

lookup_executable() {
    local path
    for path in "$@"; do
        if [[ -x "${path}" ]]; then
            echo "${path}"
            return 0
        fi
    done
    return 1
}

is_wsl() {
    [[ "${HOST_OS}" == "Linux" && "${KERNEL_RELEASE}" == *[Mm]icrosoft* ]]
}

print_help() {
    cat <<'EOF'
Usage: ./build.sh [build|test|clean|opt]

Commands:
  build   Configure and build the project (default)
  test    Configure, build, and run ctest
  clean   Remove BUILD_DIR, then configure and build again
  opt     Configure, build, and run a small opt demo

Environment overrides:
  BUILD_DIR, CMAKE_BUILD_TYPE, GENERATOR, PARALLELISM
  LLVM_CONFIG, LLVM_DIR, CC, CXX
  LEARN_LLVM_ENABLE_ONE_BACKEND, BUILD_TESTING
EOF
}

detect_llvm() {
    local preferred_versions=(17 16 15 14)
    local path_candidates=()
    local version cmd formula brew_prefix

    if [[ -n "${LLVM_CONFIG:-}" ]]; then
        [[ -x "${LLVM_CONFIG}" ]] || die "LLVM_CONFIG is set but not executable: ${LLVM_CONFIG}"
        echo "${LLVM_CONFIG}"
        return 0
    fi

    if [[ -n "${LLVM_DIR:-}" ]]; then
        local derived_bin
        derived_bin="$(cd "${LLVM_DIR}/../../../bin" 2>/dev/null && pwd || true)"
        if [[ -n "${derived_bin}" && -x "${derived_bin}/llvm-config" ]]; then
            echo "${derived_bin}/llvm-config"
            return 0
        fi
    fi

    for version in "${preferred_versions[@]}"; do
        path_candidates+=(
            "/usr/lib/llvm-${version}/bin/llvm-config"
            "/usr/local/opt/llvm@${version}/bin/llvm-config"
            "/opt/homebrew/opt/llvm@${version}/bin/llvm-config"
            "/home/linuxbrew/.linuxbrew/opt/llvm@${version}/bin/llvm-config"
        )
    done
    path_candidates+=(
        "/usr/local/opt/llvm/bin/llvm-config"
        "/opt/homebrew/opt/llvm/bin/llvm-config"
        "/home/linuxbrew/.linuxbrew/opt/llvm/bin/llvm-config"
    )

    for version in "${preferred_versions[@]}"; do
        cmd="$(lookup_cmd "llvm-config-${version}" || true)"
        if [[ -n "${cmd}" ]]; then
            echo "${cmd}"
            return 0
        fi
    done

    cmd="$(lookup_cmd llvm-config || true)"
    if [[ -n "${cmd}" ]]; then
        echo "${cmd}"
        return 0
    fi

    cmd="$(lookup_executable "${path_candidates[@]}" || true)"
    if [[ -n "${cmd}" ]]; then
        echo "${cmd}"
        return 0
    fi

    if have_cmd brew; then
        for formula in llvm@17 llvm@16 llvm@15 llvm@14 llvm; do
            brew_prefix="$(brew --prefix "${formula}" 2>/dev/null || true)"
            if [[ -n "${brew_prefix}" && -x "${brew_prefix}/bin/llvm-config" ]]; then
                echo "${brew_prefix}/bin/llvm-config"
                return 0
            fi
        done
    fi

    die "llvm-config not found. Install LLVM 14-17 and ensure llvm-config is available.
  macOS: xcode-select --install && brew install cmake ninja llvm@17
  Ubuntu/Debian: sudo apt install cmake ninja-build llvm-17-dev clang-17
  WSL: same as Ubuntu/Debian inside the WSL distro"
}

detect_parallelism() {
    if have_cmd nproc; then
        nproc
        return 0
    fi

    if have_cmd sysctl; then
        sysctl -n hw.ncpu
        return 0
    fi

    if have_cmd getconf; then
        getconf _NPROCESSORS_ONLN
        return 0
    fi

    echo 4
}

detect_generator() {
    local generator_override="${GENERATOR:-}"
    if [[ -n "${generator_override}" ]]; then
        echo "${generator_override};"
        return 0
    fi

    if have_cmd ninja; then
        echo "Ninja;$(command -v ninja)"
        return 0
    fi

    if have_cmd ninja-build; then
        echo "Ninja;$(command -v ninja-build)"
        return 0
    fi

    echo "Unix Makefiles;"
}

detect_tool_from_llvm_bin() {
    local tool_name="$1"
    local tool_path="${LLVM_BIN_DIR}/${tool_name}"
    if [[ -x "${tool_path}" ]]; then
        echo "${tool_path}"
        return 0
    fi

    if have_cmd "${tool_name}"; then
        command -v "${tool_name}"
        return 0
    fi

    return 1
}

detect_plugin_artifact() {
    local plugin_name="$1"
    local candidate
    for candidate in \
        "${BUILD_DIR}/lib/${plugin_name}.so" \
        "${BUILD_DIR}/lib/${plugin_name}.dylib"; do
        if [[ -f "${candidate}" ]]; then
            echo "${candidate}"
            return 0
        fi
    done
    return 1
}

lookup_compiler_pair() {
    local cc_name="$1"
    local cxx_name="$2"
    local cc_path cxx_path

    cc_path="$(lookup_cmd "${cc_name}" || true)"
    cxx_path="$(lookup_cmd "${cxx_name}" || true)"
    if [[ -n "${cc_path}" && -n "${cxx_path}" ]]; then
        echo "${cc_path};${cxx_path}"
        return 0
    fi

    return 1
}

detect_compilers() {
    local llvm_clang llvm_clangxx xcrun_clang xcrun_clangxx pair

    if [[ -n "${CC:-}" || -n "${CXX:-}" ]]; then
        [[ -n "${CC:-}" && -n "${CXX:-}" ]] || die "set both CC and CXX together, or leave both unset"
        have_cmd "${CC}" || die "CC not found: ${CC}"
        have_cmd "${CXX}" || die "CXX not found: ${CXX}"
        printf '%s;%s\n' "$(command -v "${CC}")" "$(command -v "${CXX}")"
        return 0
    fi

    if [[ "${HOST_OS}" == "Darwin" ]]; then
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

    for pair in \
        "gcc:g++" \
        "gcc-${LLVM_MAJOR}:g++-${LLVM_MAJOR}" \
        "clang:clang++" \
        "clang-${LLVM_MAJOR}:clang++-${LLVM_MAJOR}" \
        "cc:c++"; do
        if lookup_compiler_pair "${pair%%:*}" "${pair##*:}" >/dev/null 2>&1; then
            lookup_compiler_pair "${pair%%:*}" "${pair##*:}"
            return 0
        fi
    done

    die "no suitable C/C++ compiler pair found"
}

CMD="${1:-build}"
case "${CMD}" in
    build|test|clean|opt)
        ;;
    -h|--help|help)
        print_help
        exit 0
        ;;
    *)
        die "unknown command: ${CMD} (use --help for usage)"
        ;;
esac

LLVM_CONFIG="$(detect_llvm)"
LLVM_BIN_DIR="$(cd "$(dirname "${LLVM_CONFIG}")" && pwd)"
LLVM_CMAKE_DIR="$("${LLVM_CONFIG}" --cmakedir)"
LLVM_VERSION="$("${LLVM_CONFIG}" --version)"
LLVM_MAJOR="${LLVM_VERSION%%.*}"
PARALLELISM="${PARALLELISM:-$(detect_parallelism)}"
IFS=';' read -r GENERATOR GENERATOR_PROGRAM <<< "$(detect_generator)"
IFS=';' read -r CC CXX <<< "$(detect_compilers)"

echo "Using LLVM ${LLVM_VERSION} (${LLVM_CMAKE_DIR})"
echo "Using LLVM tools from ${LLVM_BIN_DIR}"
echo "Using generator: ${GENERATOR}"
if [[ -n "${GENERATOR_PROGRAM:-}" ]]; then
    echo "Using generator program: ${GENERATOR_PROGRAM}"
fi
echo "Using C compiler: ${CC}"
echo "Using C++ compiler: ${CXX}"
if is_wsl; then
    echo "Detected environment: WSL"
fi

if [[ "${LLVM_MAJOR}" -lt 14 || "${LLVM_MAJOR}" -gt 17 ]]; then
    warn "this repository is tested with LLVM 14-17; LLVM ${LLVM_VERSION} may not build cleanly"
fi

if [[ "${CMD}" == "clean" ]]; then
    echo "Removing ${BUILD_DIR}..."
    rm -rf "${BUILD_DIR}"
    CMD="build"
fi

echo "Configuring..."
cmake_args=(
    -S .
    -B "${BUILD_DIR}"
    -DLLVM_DIR="${LLVM_CMAKE_DIR}"
    -DCMAKE_C_COMPILER="${CC}"
    -DCMAKE_CXX_COMPILER="${CXX}"
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
    -DLEARN_LLVM_ENABLE_ONE_BACKEND="${ENABLE_ONE_BACKEND}"
    -DBUILD_TESTING="${BUILD_TESTING_OPTION}"
    -G "${GENERATOR}"
)
if [[ -n "${GENERATOR_PROGRAM:-}" ]]; then
    cmake_args+=(-DCMAKE_MAKE_PROGRAM="${GENERATOR_PROGRAM}")
fi
cmake "${cmake_args[@]}"

echo "Building..."
cmake --build "${BUILD_DIR}" --parallel "${PARALLELISM}"

if [[ "${CMD}" == "test" ]]; then
    echo "Running tests..."
    (cd "${BUILD_DIR}" && ctest --output-on-failure --parallel "${PARALLELISM}")
fi

if [[ "${CMD}" == "opt" ]]; then
    KOTO_PLUGIN="$(detect_plugin_artifact Kotoamatsukami || true)"
    [[ -n "${KOTO_PLUGIN}" ]] || die "Kotoamatsukami plugin not found under ${BUILD_DIR}/lib (expected .so or .dylib), build first"

    DEMO_DIR="$(mktemp -d "${TMPDIR:-/tmp}/koto_demo.XXXXXX")"
    CLANG_BIN="$(detect_tool_from_llvm_bin clang || true)"
    OPT_BIN="$(detect_tool_from_llvm_bin opt || true)"
    [[ -n "${CLANG_BIN}" ]] || die "clang not found"
    [[ -n "${OPT_BIN}" ]] || die "opt not found"

    echo "Generating IR from example/test.c..."
    "${CLANG_BIN}" -O1 -S -emit-llvm ./example/test.c -o "${DEMO_DIR}/test_O1.ll"
    echo "Applying flatten pass..."
    "${OPT_BIN}" -load-pass-plugin "${KOTO_PLUGIN}" \
        -passes=flatten \
        -S "${DEMO_DIR}/test_O1.ll" \
        -o "${DEMO_DIR}/test_flat.ll"
    echo "Done -> ${DEMO_DIR}/test_flat.ll"
fi

echo "build.sh done"
