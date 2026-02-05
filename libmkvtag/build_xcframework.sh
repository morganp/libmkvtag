#!/bin/bash
#
# build_xcframework.sh - Build libmkvtag as an XCFramework for iOS and macOS
#
# Usage: ./build_xcframework.sh [--clean]
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
OUTPUT_DIR="${SCRIPT_DIR}/output"
FRAMEWORK_NAME="mkvtag"

# Deployment targets
MACOS_DEPLOYMENT_TARGET="10.15"
IOS_DEPLOYMENT_TARGET="13.0"

# Clean if requested
if [[ "${1:-}" == "--clean" ]]; then
    echo "Cleaning build directory..."
    rm -rf "${BUILD_DIR}" "${OUTPUT_DIR}"
fi

mkdir -p "${BUILD_DIR}" "${OUTPUT_DIR}"

# Helper function to build for a specific platform
build_platform() {
    local platform="$1"
    local arch="$2"
    local sysroot="$3"
    local deployment_target="$4"
    local extra_flags="${5:-}"
    local build_subdir="${BUILD_DIR}/${platform}-${arch}"

    echo "=== Building for ${platform} (${arch}) ==="

    mkdir -p "${build_subdir}"

    local cmake_flags=(
        -DCMAKE_C_COMPILER="$(xcrun --find clang)"
        -DCMAKE_AR="$(xcrun --find ar)"
        -DCMAKE_RANLIB="$(xcrun --find ranlib)"
        -DCMAKE_OSX_SYSROOT="${sysroot}"
        -DCMAKE_OSX_ARCHITECTURES="${arch}"
        -DCMAKE_C_FLAGS="${extra_flags}"
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX="${build_subdir}/install"
    )

    if [[ "${platform}" == "ios"* ]]; then
        cmake_flags+=(
            -DCMAKE_OSX_DEPLOYMENT_TARGET="${IOS_DEPLOYMENT_TARGET}"
            -DCMAKE_SYSTEM_NAME=iOS
        )
        if [[ "${platform}" == "ios-simulator" ]]; then
            cmake_flags+=(
                -DCMAKE_OSX_SYSROOT="iphonesimulator"
            )
        else
            cmake_flags+=(
                -DCMAKE_OSX_SYSROOT="iphoneos"
            )
        fi
    else
        cmake_flags+=(
            -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOS_DEPLOYMENT_TARGET}"
        )
    fi

    cmake -S "${SCRIPT_DIR}" -B "${build_subdir}" "${cmake_flags[@]}"
    cmake --build "${build_subdir}" --config Release -- -j"$(sysctl -n hw.ncpu)"
    cmake --install "${build_subdir}" --config Release
}

# Get SDK paths
MACOS_SDK="$(xcrun --sdk macosx --show-sdk-path)"
IOS_SDK="$(xcrun --sdk iphoneos --show-sdk-path)"
IOS_SIM_SDK="$(xcrun --sdk iphonesimulator --show-sdk-path)"

# Build for each platform
build_platform "macos" "arm64;x86_64" "${MACOS_SDK}" "${MACOS_DEPLOYMENT_TARGET}"
build_platform "ios" "arm64" "${IOS_SDK}" "${IOS_DEPLOYMENT_TARGET}"
build_platform "ios-simulator" "arm64;x86_64" "${IOS_SIM_SDK}" "${IOS_DEPLOYMENT_TARGET}"

# Create header directory structure for the framework
echo "=== Preparing framework headers ==="

create_headers_dir() {
    local dir="$1"
    mkdir -p "${dir}/Headers/mkvtag"
    cp "${SCRIPT_DIR}/include/mkvtag/mkvtag.h" "${dir}/Headers/mkvtag/"
    cp "${SCRIPT_DIR}/include/mkvtag/mkvtag_types.h" "${dir}/Headers/mkvtag/"
    cp "${SCRIPT_DIR}/include/mkvtag/mkvtag_error.h" "${dir}/Headers/mkvtag/"
    cp "${SCRIPT_DIR}/include/mkvtag/module.modulemap" "${dir}/Headers/mkvtag/"

    # Create an umbrella header at the top level too
    cat > "${dir}/Headers/mkvtag.h" << 'UMBRELLA_EOF'
/*
 * mkvtag.h - Umbrella header for mkvtag framework
 */
#include "mkvtag/mkvtag.h"
UMBRELLA_EOF

    # Create top-level module map
    cat > "${dir}/Headers/module.modulemap" << 'MODMAP_EOF'
framework module mkvtag {
    umbrella header "mkvtag.h"
    export *
    module * { export * }
}
MODMAP_EOF
}

# Create framework bundles for each platform
for platform in macos-arm64\;x86_64 ios-arm64 ios-simulator-arm64\;x86_64; do
    platform_dir="${BUILD_DIR}/${platform}/framework"
    mkdir -p "${platform_dir}/${FRAMEWORK_NAME}.framework"
    create_headers_dir "${platform_dir}/${FRAMEWORK_NAME}.framework"
    cp "${BUILD_DIR}/${platform}/install/lib/libmkvtag.a" \
       "${platform_dir}/${FRAMEWORK_NAME}.framework/mkvtag"
done

# Create XCFramework
echo "=== Creating XCFramework ==="

XCFRAMEWORK_PATH="${OUTPUT_DIR}/${FRAMEWORK_NAME}.xcframework"
rm -rf "${XCFRAMEWORK_PATH}"

xcodebuild -create-xcframework \
    -framework "${BUILD_DIR}/macos-arm64;x86_64/framework/${FRAMEWORK_NAME}.framework" \
    -framework "${BUILD_DIR}/ios-arm64/framework/${FRAMEWORK_NAME}.framework" \
    -framework "${BUILD_DIR}/ios-simulator-arm64;x86_64/framework/${FRAMEWORK_NAME}.framework" \
    -output "${XCFRAMEWORK_PATH}"

echo ""
echo "=== XCFramework created successfully ==="
echo "Output: ${XCFRAMEWORK_PATH}"
echo ""
echo "To use in Xcode:"
echo "  1. Drag ${FRAMEWORK_NAME}.xcframework into your project"
echo "  2. Import in Swift: import mkvtag"
echo "  3. Use the C API directly"
