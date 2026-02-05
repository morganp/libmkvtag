cmake_minimum_required(VERSION 3.15)
project(mkvtag VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Source files
set(MKVTAG_SOURCES
    src/ebml/ebml_vint.c
    src/ebml/ebml_reader.c
    src/ebml/ebml_writer.c
    src/io/file_io.c
    src/mkv/mkv_parser.c
    src/mkv/mkv_tags.c
    src/mkv/mkv_seekhead.c
    src/util/buffer.c
    src/util/string_util.c
    src/mkvtag.c
)

# Public headers
set(MKVTAG_PUBLIC_HEADERS
    include/mkvtag/mkvtag.h
    include/mkvtag/mkvtag_types.h
    include/mkvtag/mkvtag_error.h
)

# Static library
add_library(mkvtag STATIC ${MKVTAG_SOURCES})

target_include_directories(mkvtag
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compiler warnings
target_compile_options(mkvtag PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
)

# Set properties for the library
set_target_properties(mkvtag PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${MKVTAG_PUBLIC_HEADERS}"
    POSITION_INDEPENDENT_CODE ON
)

# Apple framework-specific settings
if(APPLE)
    set_target_properties(mkvtag PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
        XCODE_ATTRIBUTE_DEFINES_MODULE YES
    )

    # Deployment targets
    if(CMAKE_OSX_SYSROOT MATCHES "iphoneos")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "iOS deployment target")
    elseif(CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "iOS Simulator deployment target")
    else()
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "macOS deployment target")
    endif()
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS mkvtag
    EXPORT mkvtag-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mkvtag
)

# Install module map for Swift interop
if(APPLE)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/mkvtag/module.modulemap
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mkvtag
    )
endif()

# Export targets
install(EXPORT mkvtag-targets
    FILE mkvtag-config.cmake
    NAMESPACE mkvtag::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mkvtag
)

# Build tests if enabled
option(MKVTAG_BUILD_TESTS "Build tests" OFF)
if(MKVTAG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
