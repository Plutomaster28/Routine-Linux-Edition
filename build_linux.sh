#!/usr/bin/env bash
# Configure, build, and test the complete native Linux/WSL distribution.

set -euo pipefail

ROUTINE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROUTINE_BUILD_DIR="${ROUTINE_ROOT}/build-linux"

cd "${ROUTINE_ROOT}"

if [[ "$(uname -s)" != "Linux" ]]; then
    echo "build_linux.sh must be run on Linux or WSL."
    exit 1
fi

missing=()
for package in libcurl libwebsockets yaml-cpp openssl; do
    if ! pkg-config --exists "${package}"; then
        missing+=("${package}")
    fi
done

lua_package=""
for candidate in lua lua5.4 lua-5.4 lua54 lua5.3 lua-5.3 lua53 \
                 lua5.2 lua-5.2 lua52 lua5.1 lua-5.1 lua51; do
    if pkg-config --exists "${candidate}"; then
        lua_package="${candidate}"
        break
    fi
done
if [[ -z "${lua_package}" ]]; then
    missing+=("lua")
fi

if (( ${#missing[@]} > 0 )); then
    echo "Missing Linux development packages: ${missing[*]}"
    echo "Run ./install_deps_linux.sh first."
    exit 1
fi

echo "Configuring Linux bot kernel..."
cmake -S . -B "${ROUTINE_BUILD_DIR}/kernel" -G Ninja
cmake --build "${ROUTINE_BUILD_DIR}/kernel"
ctest --test-dir "${ROUTINE_BUILD_DIR}/kernel" --output-on-failure

echo "Building and testing extensions..."
cmake -S lib -B "${ROUTINE_BUILD_DIR}/extensions" -G Ninja
cmake --build "${ROUTINE_BUILD_DIR}/extensions"
ctest --test-dir "${ROUTINE_BUILD_DIR}/extensions" --output-on-failure

echo "Building and testing modules..."
cmake -S modules -B "${ROUTINE_BUILD_DIR}/modules" -G Ninja
cmake --build "${ROUTINE_BUILD_DIR}/modules"
ctest --test-dir "${ROUTINE_BUILD_DIR}/modules" --output-on-failure

echo "Installing runtime module and extension libraries..."
for artifact in simd_extension local_economy_extension; do
    cp -f "${ROUTINE_BUILD_DIR}/extensions/${artifact}.so" \
          "${ROUTINE_ROOT}/lib/${artifact}.so"
done
for artifact in fortran_math local_economy_module; do
    cp -f "${ROUTINE_BUILD_DIR}/modules/${artifact}.so" \
          "${ROUTINE_ROOT}/modules/${artifact}.so"
done

mkdir -p data logs
if [[ ! -f config.json ]]; then
    cp config.example.json config.json
fi
chmod 600 config.json

echo
echo "Linux build is ready."
echo "Executable: ${ROUTINE_BUILD_DIR}/kernel/routine"
echo "Ensure ${ROUTINE_ROOT}/config.json contains your bot token."
echo "Then run from the project root:"
echo "  ./build-linux/kernel/routine"
