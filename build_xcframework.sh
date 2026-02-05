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

# Source files
SOURCES=(
    src/ebml/ebml_vint.c
    src/ebml/ebml_reader.c
    src/ebml/ebml_writer.c
    src/io/file_io.c
    src/util/buffer.c
    src/util/string_util.c
    src/mkv/mkv_parser.c
    src/mkv/mkv_tags.c
    src/mkv/mkv_seekhead.c
    src/mkvtag.c
)

CFLAGS=(-std=c11 -Wall -Wextra -Wpedantic -Wno-unused-parameter -O2)
INCLUDES=("-I${SCRIPT_DIR}/include" "-I${SCRIPT_DIR}/src")

# Clean if requested
if [[ "${1:-}" == "--clean" ]]; then
    echo "Cleaning build directory..."
    rm -rf "${BUILD_DIR}" "${OUTPUT_DIR}"
fi

mkdir -p "${BUILD_DIR}" "${OUTPUT_DIR}"

# Build static library for a platform/arch combination
build_platform() {
    local platform="$1"
    local archs="$2"
    local sdk="$3"
    local min_version_flag="$4"
    local build_subdir="${BUILD_DIR}/${platform}"

    echo ""
    echo "=== Building for ${platform} (${archs}) ==="

    mkdir -p "${build_subdir}/obj"

    local sdk_path
    sdk_path="$(xcrun --sdk "${sdk}" --show-sdk-path)"

    local arch_flags=""
    for arch in ${archs}; do
        arch_flags="${arch_flags} -arch ${arch}"
    done

    # Compile each source file
    local obj_files=()
    for src in "${SOURCES[@]}"; do
        local base
        base="$(basename "${src}" .c)"
        local obj="${build_subdir}/obj/${base}.o"
        obj_files+=("${obj}")

        xcrun clang "${CFLAGS[@]}" "${INCLUDES[@]}" \
            -isysroot "${sdk_path}" \
            ${arch_flags} \
            "${min_version_flag}" \
            -c "${SCRIPT_DIR}/${src}" \
            -o "${obj}"
    done

    # Create static library
    xcrun ar rcs "${build_subdir}/libmkvtag.a" "${obj_files[@]}"
    echo "  Built: ${build_subdir}/libmkvtag.a"
}

# Detect available SDKs
AVAILABLE_PLATFORMS=()

if xcrun --sdk macosx --show-sdk-path &>/dev/null; then
    build_platform "macos" "arm64 x86_64" "macosx" "-mmacosx-version-min=${MACOS_DEPLOYMENT_TARGET}"
    AVAILABLE_PLATFORMS+=("macos")
else
    echo "WARNING: macOS SDK not found, skipping"
fi

if xcrun --sdk iphoneos --show-sdk-path &>/dev/null; then
    build_platform "ios-device" "arm64" "iphoneos" "-miphoneos-version-min=${IOS_DEPLOYMENT_TARGET}"
    AVAILABLE_PLATFORMS+=("ios-device")
else
    echo "WARNING: iOS SDK not found, skipping (install Xcode for iOS support)"
fi

if xcrun --sdk iphonesimulator --show-sdk-path &>/dev/null; then
    build_platform "ios-simulator" "arm64 x86_64" "iphonesimulator" "-mios-simulator-version-min=${IOS_DEPLOYMENT_TARGET}"
    AVAILABLE_PLATFORMS+=("ios-simulator")
else
    echo "WARNING: iOS Simulator SDK not found, skipping (install Xcode for iOS support)"
fi

if [[ ${#AVAILABLE_PLATFORMS[@]} -eq 0 ]]; then
    echo "ERROR: No SDKs available. Install Xcode or Command Line Tools."
    exit 1
fi

# Create framework bundles for each platform
echo ""
echo "=== Creating framework bundles ==="

create_framework() {
    local platform="$1"
    local fw_dir="${BUILD_DIR}/${platform}/framework/${FRAMEWORK_NAME}.framework"

    mkdir -p "${fw_dir}/Headers/mkvtag"
    mkdir -p "${fw_dir}/Modules"

    # Copy public headers into framework
    cp "${SCRIPT_DIR}/include/mkvtag/mkvtag.h"        "${fw_dir}/Headers/mkvtag/"
    cp "${SCRIPT_DIR}/include/mkvtag/mkvtag_types.h"   "${fw_dir}/Headers/mkvtag/"
    cp "${SCRIPT_DIR}/include/mkvtag/mkvtag_error.h"   "${fw_dir}/Headers/mkvtag/"

    # Umbrella header
    cat > "${fw_dir}/Headers/mkvtag.h" << 'EOF'
/*
 * mkvtag.h - Umbrella header for mkvtag framework
 */
#include "mkvtag/mkvtag.h"
EOF

    # Module map
    cat > "${fw_dir}/Modules/module.modulemap" << 'EOF'
framework module mkvtag {
    umbrella header "mkvtag.h"
    export *
    module * { export * }
}
EOF

    # Copy static library as framework binary
    cp "${BUILD_DIR}/${platform}/libmkvtag.a" "${fw_dir}/${FRAMEWORK_NAME}"

    # Info.plist
    cat > "${fw_dir}/Info.plist" << PLISTEOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>${FRAMEWORK_NAME}</string>
    <key>CFBundleIdentifier</key>
    <string>com.libmkvtag.${FRAMEWORK_NAME}</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>CFBundlePackageType</key>
    <string>FMWK</string>
</dict>
</plist>
PLISTEOF

    echo "  Created: ${fw_dir}"
}

for platform in "${AVAILABLE_PLATFORMS[@]}"; do
    create_framework "${platform}"
done

# Create XCFramework (requires xcodebuild from full Xcode)
echo ""
echo "=== Creating XCFramework ==="

XCFRAMEWORK_PATH="${OUTPUT_DIR}/${FRAMEWORK_NAME}.xcframework"
rm -rf "${XCFRAMEWORK_PATH}"

FW_ARGS=()
for platform in "${AVAILABLE_PLATFORMS[@]}"; do
    FW_ARGS+=(-framework "${BUILD_DIR}/${platform}/framework/${FRAMEWORK_NAME}.framework")
done

if xcodebuild -version &>/dev/null; then
    xcodebuild -create-xcframework \
        "${FW_ARGS[@]}" \
        -output "${XCFRAMEWORK_PATH}"
else
    echo ""
    echo "WARNING: xcodebuild not available (requires full Xcode install)."
    echo "Static libraries were built successfully for: ${AVAILABLE_PLATFORMS[*]}"
    echo ""
    for platform in "${AVAILABLE_PLATFORMS[@]}"; do
        echo "  Framework: ${BUILD_DIR}/${platform}/framework/${FRAMEWORK_NAME}.framework"
    done
    echo ""
    echo "Install Xcode to create the XCFramework bundle."
    exit 0
fi

echo ""
echo "=== XCFramework created successfully ==="
echo "Output: ${XCFRAMEWORK_PATH}"
echo ""
echo "To use in Xcode:"
echo "  1. Drag ${FRAMEWORK_NAME}.xcframework into your project"
echo "  2. Import in Swift: import mkvtag"
echo "  3. Use the C API directly"
