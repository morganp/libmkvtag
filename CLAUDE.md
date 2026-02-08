# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

Build library:
```sh
cd build && xcrun clang -c -std=c11 -Wall -Wextra -Wpedantic -Wno-unused-parameter -I ../include -I ../src \
    ../src/ebml/ebml_vint.c ../src/ebml/ebml_reader.c ../src/ebml/ebml_writer.c \
    ../src/io/file_io.c ../src/util/buffer.c ../src/util/string_util.c \
    ../src/mkv/mkv_parser.c ../src/mkv/mkv_tags.c ../src/mkv/mkv_seekhead.c \
    ../src/mkvtag.c \
    && xcrun ar rcs libmkvtag.a ebml_vint.o ebml_reader.o ebml_writer.o file_io.o buffer.o string_util.o mkv_parser.o mkv_tags.o mkv_seekhead.o mkvtag.o
```

Build XCFramework (macOS + iOS):
```sh
./build_xcframework.sh
```

## Testing

Build and run tests:
```sh
cmake -S . -B build -DMKVTAG_BUILD_TESTS=ON && cmake --build build && ctest --test-dir build --output-on-failure
```

## Architecture

Pure C11 static library for reading/writing Matroska (MKV/MKA/WebM) metadata tags. No external dependencies (POSIX only). API is compatible with [liboggtag](https://github.com/morganp/liboggtag) and [libmp3tag](https://github.com/morganp/libmp3tag).

### Layers

- **Public API** (`include/mkvtag/`) — `mkvtag.h` (functions), `mkvtag_types.h` (structs/enums), `mkvtag_error.h` (error codes), `module.modulemap` (Swift/Clang)
- **Main implementation** (`src/mkvtag.c`) — Context lifecycle, tag read/write orchestration, collection building, simple get/set convenience API
- **EBML** (`src/ebml/`) — Variable-length integer codec (`ebml_vint`), stream-based element parsing (`ebml_reader`), element serialization (`ebml_writer`), element ID constants (`ebml_ids.h`)
- **MKV** (`src/mkv/`) — Header validation and segment parsing (`mkv_parser`), tag parsing and serialization (`mkv_tags`), SeekHead entry management (`mkv_seekhead`)
- **I/O** (`src/io/`) — Buffered POSIX file I/O (8KB read buffer, lazy seek)
- **Util** (`src/util/`) — Dynamic byte buffer (`dyn_buffer_t`), string helpers (duplication, case-insensitive compare)

### Write Strategies

Three strategies to minimize file modification, tried in order:

1. **In-place replacement**: If new Tags element fits within existing Tags + adjacent Void element, writes in-place and fills remainder with a Void element
2. **Void replacement**: Replaces the largest Void element in the file if it has enough space
3. **Append to Segment**: Appends Tags at end of Segment with automatic Segment size update

### SeekHead Handling

The parser uses SeekHead entries (if present) for fast lookup of Tags and other top-level elements, avoiding a full file scan. When writing, SeekHead entries are updated to reflect new Tags element positions.
