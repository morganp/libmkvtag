# libmkvtag

A pure C library for reading and writing Matroska (MKV/MKA/WebM) file metadata tags without loading entire files into memory.

Designed for embedding in iOS and macOS apps via XCFramework, with a clean C API that works directly from Swift through a module map.

## Features

- Read and write MKV/MKA/WebM metadata tags (title, artist, comment, etc.)
- Memory-efficient: uses buffered I/O and skips media data (Clusters)
- Three write strategies to minimize file modification:
  1. In-place replacement of existing Tags + adjacent Void elements
  2. Replacement of the largest Void element in the file
  3. Append to end of Segment with automatic size update
- SeekHead-aware: uses the index for fast element lookup
- No dependencies beyond POSIX and the C standard library
- Compiles cleanly with `-Wall -Wextra -Wpedantic` (C11)

## Quick Start

```c
#include <mkvtag/mkvtag.h>

// Read a tag
mkvtag_context_t *ctx = mkvtag_create(NULL);
mkvtag_open(ctx, "video.mkv");

char title[256];
if (mkvtag_read_tag_string(ctx, "TITLE", title, sizeof(title)) == MKVTAG_OK) {
    printf("Title: %s\n", title);
}
mkvtag_destroy(ctx);

// Write a tag
ctx = mkvtag_create(NULL);
mkvtag_open_rw(ctx, "video.mkv");
mkvtag_set_tag_string(ctx, "TITLE", "New Title");
mkvtag_destroy(ctx);
```

For full control over tag structure (targets, multiple tags, nested tags):

```c
mkvtag_context_t *ctx = mkvtag_create(NULL);
mkvtag_open_rw(ctx, "video.mkv");

mkvtag_collection_t *coll = mkvtag_collection_create(ctx);
mkvtag_tag_t *tag = mkvtag_collection_add_tag(ctx, coll, MKVTAG_TARGET_ALBUM);
mkvtag_tag_add_simple(ctx, tag, "TITLE", "My Movie");
mkvtag_tag_add_simple(ctx, tag, "ARTIST", "Director Name");
mkvtag_tag_add_simple(ctx, tag, "DATE_RELEASED", "2024");

mkvtag_write_tags(ctx, coll);
mkvtag_collection_free(ctx, coll);
mkvtag_destroy(ctx);
```

## Dependencies

- [libtag_common](https://github.com/morganp/libtag_common) — shared I/O, buffer, and string utilities (included as git submodule)

## Building

Clone with submodules:

```sh
git clone --recursive https://github.com/morganp/libmkvtag.git
```

If already cloned:

```sh
git submodule update --init
```

### XCFramework for iOS/macOS

```sh
./build_xcframework.sh
# Output: output/mkvtag.xcframework
```

Targets: macOS 10.15+ (arm64, x86_64), iOS 13.0+ (arm64), iOS Simulator (arm64, x86_64).

### Manual build

```sh
xcrun clang -c -std=c11 -Wall -Wextra -Wpedantic -I include -I src -I deps/libtag_common/include \
    src/ebml/ebml_vint.c src/ebml/ebml_reader.c src/ebml/ebml_writer.c \
    src/mkv/mkv_parser.c src/mkv/mkv_tags.c src/mkv/mkv_seekhead.c \
    src/mkvtag.c
xcrun ar rcs libmkvtag.a *.o
```

## Testing

```sh
cmake -S . -B build -DMKVTAG_BUILD_TESTS=ON && cmake --build build && ctest --test-dir build --output-on-failure
```

## API Reference

### Context Lifecycle

| Function | Description |
|---|---|
| `mkvtag_create(allocator)` | Create a context (pass `NULL` for default allocator) |
| `mkvtag_destroy(ctx)` | Destroy context and close any open file |
| `mkvtag_open(ctx, path)` | Open a Matroska file (.mkv, .mka, .webm) for reading |
| `mkvtag_open_rw(ctx, path)` | Open a Matroska file (.mkv, .mka, .webm) for reading and writing |
| `mkvtag_close(ctx)` | Close the current file |

### Tag Reading

| Function | Description |
|---|---|
| `mkvtag_read_tags(ctx, &tags)` | Read all tags into a collection |
| `mkvtag_read_tag_string(ctx, name, buf, size)` | Read a single tag value by name (case-insensitive) |

### Tag Writing

| Function | Description |
|---|---|
| `mkvtag_write_tags(ctx, tags)` | Replace all tags with a collection |
| `mkvtag_set_tag_string(ctx, name, value)` | Set a single tag (creates or updates) |
| `mkvtag_remove_tag(ctx, name)` | Remove a tag by name |

### Collection Building

| Function | Description |
|---|---|
| `mkvtag_collection_create(ctx)` | Create an empty tag collection |
| `mkvtag_collection_free(ctx, coll)` | Free a tag collection |
| `mkvtag_collection_add_tag(ctx, coll, type)` | Add a tag with a target type |
| `mkvtag_tag_add_simple(ctx, tag, name, value)` | Add a name/value pair to a tag |
| `mkvtag_simple_tag_add_nested(ctx, parent, name, value)` | Add a nested simple tag |
| `mkvtag_simple_tag_set_language(ctx, stag, lang)` | Set language on a simple tag |
| `mkvtag_tag_add_track_uid(ctx, tag, uid)` | Scope a tag to a specific track |

### Error Handling

All functions return `MKVTAG_OK` (0) on success or a negative error code. Use `mkvtag_strerror(err)` to get a human-readable message.

## Project Structure

```
├── include/mkvtag/         Public headers + Swift module map
│   ├── mkvtag.h            Main API
│   ├── mkvtag_types.h      Type definitions
│   ├── mkvtag_error.h      Error codes
│   └── module.modulemap    Swift interop
├── deps/
│   └── libtag_common/      Shared I/O, buffer & string utilities (submodule)
├── src/
│   ├── ebml/               EBML format layer
│   │   ├── ebml_vint       Variable-length integer codec
│   │   ├── ebml_reader     Stream-based element parsing
│   │   ├── ebml_writer     Element serialization
│   │   └── ebml_ids.h      Element ID constants
│   ├── mkv/                Matroska structure layer
│   │   ├── mkv_parser      Header validation, segment parsing, SeekHead
│   │   ├── mkv_tags        Tag parsing and serialization
│   │   └── mkv_seekhead    SeekHead entry management
│   └── mkvtag.c            Main API implementation
└── build_xcframework.sh
```

## License

MIT
