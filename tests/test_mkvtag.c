/*
 * test_mkvtag.c - Test suite for libmkvtag
 *
 * Generates minimal Matroska (.mkv) and WebM (.webm) test files,
 * then exercises the tag read/write API on each.
 */

#include <mkvtag/mkvtag.h>

#include "ebml/ebml_writer.h"
#include "ebml/ebml_ids.h"
#include "util/buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/* -- Test harness -------------------------------------------------------- */

static int g_pass = 0, g_fail = 0;

#define CHECK(cond, msg) do { \
    if (cond) { g_pass++; } \
    else { g_fail++; printf("  FAIL: %s (line %d)\n", msg, __LINE__); } \
} while (0)

#define CHECK_RC(rc, msg) CHECK((rc) == MKVTAG_OK, msg)

/* -- Helper: write buffer to fd ------------------------------------------ */

static void write_buf(int fd, const dyn_buffer_t *buf)
{
    write(fd, buf->data, buf->size);
}

/* -- Create minimal Matroska file ---------------------------------------- */

static void create_mkv_file(const char *path, const char *doctype)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return;

    /* Build EBML Header content */
    dyn_buffer_t hdr_content;
    buffer_init(&hdr_content, 64);
    ebml_write_uint_element(&hdr_content, EBML_ID_VERSION, 1);
    ebml_write_uint_element(&hdr_content, EBML_ID_READ_VERSION, 1);
    ebml_write_uint_element(&hdr_content, EBML_ID_MAX_ID_LENGTH, 4);
    ebml_write_uint_element(&hdr_content, EBML_ID_MAX_SIZE_LENGTH, 8);
    ebml_write_string_element(&hdr_content, EBML_ID_DOCTYPE, doctype);
    ebml_write_uint_element(&hdr_content, EBML_ID_DOCTYPE_VERSION, 4);
    ebml_write_uint_element(&hdr_content, EBML_ID_DOCTYPE_READ_VER, 2);

    /* EBML Header master element */
    dyn_buffer_t ebml_hdr;
    buffer_init(&ebml_hdr, 64);
    ebml_write_master_element_header(&ebml_hdr, EBML_ID_EBML, hdr_content.size);
    buffer_append(&ebml_hdr, hdr_content.data, hdr_content.size);
    buffer_free(&hdr_content);

    /* Build Segment content: Info + Void padding */

    /* Info element content */
    dyn_buffer_t info_content;
    buffer_init(&info_content, 64);
    ebml_write_uint_element(&info_content, MKV_ID_TIMECODE_SCALE, 1000000);
    ebml_write_string_element(&info_content, MKV_ID_MUXING_APP, "test");
    ebml_write_string_element(&info_content, MKV_ID_WRITING_APP, "test");

    /* Info master element */
    dyn_buffer_t info_elem;
    buffer_init(&info_elem, 64);
    ebml_write_master_element_header(&info_elem, MKV_ID_INFO, info_content.size);
    buffer_append(&info_elem, info_content.data, info_content.size);
    buffer_free(&info_content);

    /* Void element for padding (~4KB) */
    dyn_buffer_t void_elem;
    buffer_init(&void_elem, 4096);
    ebml_write_void_element(&void_elem, 4096);

    /* Segment content = Info + Void */
    uint64_t segment_content_size = info_elem.size + void_elem.size;

    /* Segment master element */
    dyn_buffer_t segment;
    buffer_init(&segment, segment_content_size + 16);
    ebml_write_master_element_header(&segment, MKV_ID_SEGMENT, segment_content_size);
    buffer_append(&segment, info_elem.data, info_elem.size);
    buffer_append(&segment, void_elem.data, void_elem.size);
    buffer_free(&info_elem);
    buffer_free(&void_elem);

    /* Write everything */
    write_buf(fd, &ebml_hdr);
    write_buf(fd, &segment);
    buffer_free(&ebml_hdr);
    buffer_free(&segment);

    close(fd);
}

static void create_mkv(const char *path)
{
    create_mkv_file(path, "matroska");
}

static void create_webm(const char *path)
{
    create_mkv_file(path, "webm");
}

/* -- Test: version ------------------------------------------------------- */

static void test_version(void)
{
    printf("Testing version ...\n");
    const char *v = mkvtag_version();
    CHECK(v != NULL, "version not NULL");
    CHECK(strlen(v) > 0, "version not empty");
    printf("\n");
}

/* -- Test: error strings ------------------------------------------------- */

static void test_error_strings(void)
{
    printf("Testing error strings ...\n");
    const char *s;

    s = mkvtag_strerror(MKVTAG_OK);
    CHECK(s != NULL && strlen(s) > 0, "strerror(OK)");

    s = mkvtag_strerror(MKVTAG_ERR_IO);
    CHECK(s != NULL && strlen(s) > 0, "strerror(IO)");

    s = mkvtag_strerror(MKVTAG_ERR_NOT_MKV);
    CHECK(s != NULL && strlen(s) > 0, "strerror(NOT_MKV)");

    s = mkvtag_strerror(MKVTAG_ERR_TAG_NOT_FOUND);
    CHECK(s != NULL && strlen(s) > 0, "strerror(TAG_NOT_FOUND)");

    s = mkvtag_strerror(-9999);
    CHECK(s != NULL && strlen(s) > 0, "strerror(unknown)");

    printf("\n");
}

/* -- Test: context lifecycle --------------------------------------------- */

static void test_context_lifecycle(void)
{
    printf("Testing context lifecycle ...\n");

    mkvtag_context_t *ctx = mkvtag_create(NULL);
    CHECK(ctx != NULL, "create context");
    CHECK(mkvtag_is_open(ctx) == 0, "not open initially");

    mkvtag_destroy(ctx);
    g_pass++; /* survived destroy */

    /* destroy NULL should not crash */
    mkvtag_destroy(NULL);
    g_pass++; /* survived destroy(NULL) */

    printf("\n");
}

/* -- Test: open invalid files -------------------------------------------- */

static void test_open_invalid(void)
{
    printf("Testing open invalid files ...\n");
    int rc;

    mkvtag_context_t *ctx = mkvtag_create(NULL);

    /* Non-existent file */
    rc = mkvtag_open(ctx, "/tmp/test_mkvtag_nonexistent_file.mkv");
    CHECK(rc != MKVTAG_OK, "open non-existent file fails");

    /* Write a plain text file and try to open it */
    const char *text_path = "/tmp/test_mkvtag_not_ebml.txt";
    int fd = open(text_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, "This is not an MKV file\n", 24);
        close(fd);
    }
    rc = mkvtag_open(ctx, text_path);
    CHECK(rc != MKVTAG_OK, "open text file fails");
    remove(text_path);

    mkvtag_destroy(ctx);
    printf("\n");
}

/* -- Test: read-only protection ------------------------------------------ */

static void test_read_only_protection(const char *path)
{
    printf("Testing read-only protection ...\n");

    mkvtag_context_t *ctx = mkvtag_create(NULL);
    int rc = mkvtag_open(ctx, path);
    CHECK_RC(rc, "open read-only");

    rc = mkvtag_set_tag_string(ctx, "TITLE", "Should Fail");
    CHECK(rc == MKVTAG_ERR_READ_ONLY, "set_tag_string fails read-only");

    mkvtag_destroy(ctx);
    printf("\n");
}

/* -- Test: format (parametric for .mkv and .webm) ------------------------ */

static void test_format(const char *path, const char *label)
{
    printf("Testing %s ...\n", label);

    mkvtag_context_t *ctx;
    int rc;
    char buf[256];

    /* Open read-write */
    ctx = mkvtag_create(NULL);
    CHECK(ctx != NULL, "create context");
    rc = mkvtag_open_rw(ctx, path);
    CHECK_RC(rc, "open_rw");

    /* Initially no user tags */
    rc = mkvtag_read_tag_string(ctx, "TITLE", buf, sizeof(buf));
    CHECK(rc == MKVTAG_ERR_TAG_NOT_FOUND || rc == MKVTAG_ERR_NO_TAGS,
          "no TITLE initially");

    /* Write some tags */
    rc = mkvtag_set_tag_string(ctx, "TITLE", "Test Title");
    CHECK_RC(rc, "set TITLE");
    rc = mkvtag_set_tag_string(ctx, "ARTIST", "Test Artist");
    CHECK_RC(rc, "set ARTIST");
    rc = mkvtag_set_tag_string(ctx, "ALBUM", "Test Album");
    CHECK_RC(rc, "set ALBUM");
    rc = mkvtag_set_tag_string(ctx, "DATE_RELEASED", "2025");
    CHECK_RC(rc, "set DATE_RELEASED");

    /* Read them back */
    rc = mkvtag_read_tag_string(ctx, "TITLE", buf, sizeof(buf));
    CHECK_RC(rc, "read TITLE");
    CHECK(strcmp(buf, "Test Title") == 0, "TITLE value");

    rc = mkvtag_read_tag_string(ctx, "ARTIST", buf, sizeof(buf));
    CHECK_RC(rc, "read ARTIST");
    CHECK(strcmp(buf, "Test Artist") == 0, "ARTIST value");

    rc = mkvtag_read_tag_string(ctx, "ALBUM", buf, sizeof(buf));
    CHECK_RC(rc, "read ALBUM");
    CHECK(strcmp(buf, "Test Album") == 0, "ALBUM value");

    rc = mkvtag_read_tag_string(ctx, "DATE_RELEASED", buf, sizeof(buf));
    CHECK_RC(rc, "read DATE_RELEASED");
    CHECK(strcmp(buf, "2025") == 0, "DATE_RELEASED value");

    /* Update a tag */
    rc = mkvtag_set_tag_string(ctx, "TITLE", "Updated Title");
    CHECK_RC(rc, "update TITLE");
    rc = mkvtag_read_tag_string(ctx, "TITLE", buf, sizeof(buf));
    CHECK_RC(rc, "read updated TITLE");
    CHECK(strcmp(buf, "Updated Title") == 0, "updated TITLE value");

    /* Remove a tag */
    rc = mkvtag_remove_tag(ctx, "DATE_RELEASED");
    CHECK_RC(rc, "remove DATE_RELEASED");
    rc = mkvtag_read_tag_string(ctx, "DATE_RELEASED", buf, sizeof(buf));
    CHECK(rc == MKVTAG_ERR_TAG_NOT_FOUND, "DATE_RELEASED removed");

    mkvtag_destroy(ctx);

    /* Reopen read-only and verify persistence */
    ctx = mkvtag_create(NULL);
    rc = mkvtag_open(ctx, path);
    CHECK_RC(rc, "reopen read-only");

    rc = mkvtag_read_tag_string(ctx, "TITLE", buf, sizeof(buf));
    CHECK_RC(rc, "persistent TITLE");
    CHECK(strcmp(buf, "Updated Title") == 0, "persistent TITLE value");

    rc = mkvtag_read_tag_string(ctx, "ARTIST", buf, sizeof(buf));
    CHECK_RC(rc, "persistent ARTIST");
    CHECK(strcmp(buf, "Test Artist") == 0, "persistent ARTIST value");

    rc = mkvtag_read_tag_string(ctx, "DATE_RELEASED", buf, sizeof(buf));
    CHECK(rc == MKVTAG_ERR_TAG_NOT_FOUND, "DATE_RELEASED still removed");

    /* Test collection read API */
    mkvtag_collection_t *coll = NULL;
    rc = mkvtag_read_tags(ctx, &coll);
    CHECK_RC(rc, "read_tags collection");
    CHECK(coll != NULL, "collection not NULL");
    if (coll && coll->tags) {
        int count = 0;
        for (mkvtag_simple_tag_t *s = coll->tags->simple_tags; s; s = s->next)
            count++;
        CHECK(count == 3, "collection has 3 tags (TITLE, ARTIST, ALBUM)");
    }

    mkvtag_destroy(ctx);

    /* Test collection write API */
    ctx = mkvtag_create(NULL);
    rc = mkvtag_open_rw(ctx, path);
    CHECK_RC(rc, "reopen rw for collection write");

    coll = mkvtag_collection_create(ctx);
    CHECK(coll != NULL, "create collection");
    mkvtag_tag_t *tag = mkvtag_collection_add_tag(ctx, coll, MKVTAG_TARGET_ALBUM);
    CHECK(tag != NULL, "add tag to collection");
    mkvtag_tag_add_simple(ctx, tag, "TITLE", "Collection Title");
    mkvtag_tag_add_simple(ctx, tag, "ARTIST", "Collection Artist");
    mkvtag_tag_add_simple(ctx, tag, "DATE_RELEASED", "2025");

    rc = mkvtag_write_tags(ctx, coll);
    CHECK_RC(rc, "write collection");
    mkvtag_collection_free(ctx, coll);

    /* Verify */
    rc = mkvtag_read_tag_string(ctx, "TITLE", buf, sizeof(buf));
    CHECK_RC(rc, "read collection TITLE");
    CHECK(strcmp(buf, "Collection Title") == 0, "collection TITLE value");

    rc = mkvtag_read_tag_string(ctx, "DATE_RELEASED", buf, sizeof(buf));
    CHECK_RC(rc, "read DATE_RELEASED");
    CHECK(strcmp(buf, "2025") == 0, "DATE_RELEASED value");

    mkvtag_destroy(ctx);
    printf("\n");
}

/* -- Main ---------------------------------------------------------------- */

int main(void)
{
    printf("libmkvtag test suite (v%s)\n\n", mkvtag_version());

    const char *mkv_path  = "/tmp/test_mkvtag.mkv";
    const char *webm_path = "/tmp/test_mkvtag.webm";

    /* Basic tests */
    test_version();
    test_error_strings();
    test_context_lifecycle();
    test_open_invalid();

    /* Format tests: MKV */
    create_mkv(mkv_path);
    test_read_only_protection(mkv_path);
    test_format(mkv_path, "Matroska (.mkv)");

    /* Format tests: WebM */
    create_webm(webm_path);
    test_format(webm_path, "WebM (.webm)");

    /* Cleanup */
    remove(mkv_path);
    remove(webm_path);

    printf("Results: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
