/*
 * Copyright (c) 2016 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl.
 *
 * Change Date: 2019-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

/**
 *
 * @verbatim
 * Revision History
 *
 * Date         Who                 Description
 * 29-08-2014   Martin Brampton     Initial implementation
 *
 * @endverbatim
 */

// To ensure that ss_info_assert asserts also when building in non-debug mode.
#if !defined(SS_DEBUG)
#define SS_DEBUG
#endif
#if defined(NDEBUG)
#undef NDEBUG
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <buffer.h>
#include <hint.h>

/**
 * Generate predefined test data
 *
 * @param count Number of bytes to generate
 * @return Pointer to @c count bytes of data
 */
uint8_t* generate_data(size_t count)
{
    uint8_t* data = malloc(count);

    srand(0);

    for (size_t i = 0; i < count; i++)
    {
        data[i] = rand() % 256;
    }

    return data;
}

size_t buffers[] =
{
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67,
    71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149
};

const int n_buffers = sizeof(buffers) / sizeof(size_t);

GWBUF* create_test_buffer()
{
    GWBUF* head = NULL;
    size_t total = 0;

    for (int i = 0; i < n_buffers; i++)
    {
        total += buffers[i];
    }

    uint8_t* data = generate_data(total);
    total = 0;

    for (int i = 0; i < sizeof(buffers) / sizeof(size_t); i++)
    {
        head = gwbuf_append(head, gwbuf_alloc_and_load(buffers[i], data + total));
        total += buffers[i];
    }

    return head;
}

int get_length_at(int n)
{
    int total = 0;

    for (int i = 0; i < n_buffers && i <= n; i++)
    {
        total += buffers[i];
    }

    return total;
}

void split_buffer(int n, int offset)
{
    int cutoff = get_length_at(n) + offset;
    GWBUF* buffer = create_test_buffer();
    int len = gwbuf_length(buffer);
    GWBUF* newbuf = gwbuf_split(&buffer, cutoff);

    ss_info_dassert(buffer && newbuf, "Both buffers should be non-NULL");
    ss_info_dassert(gwbuf_length(newbuf) == cutoff, "New buffer should be have correct length");
    ss_info_dassert(gwbuf_length(buffer) == len - cutoff, "Old buffer should be have correct length");
    gwbuf_free(buffer);
    gwbuf_free(newbuf);
}


void consume_buffer(int n, int offset)
{
    int cutoff = get_length_at(n) + offset;
    GWBUF* buffer = create_test_buffer();
    int len = gwbuf_length(buffer);
    buffer = gwbuf_consume(buffer, cutoff);

    ss_info_dassert(buffer, "Buffer should be non-NULL");
    ss_info_dassert(gwbuf_length(buffer) == len - cutoff, "Buffer should be have correct length");
    gwbuf_free(buffer);
}

void copy_buffer(int n, int offset)
{
    int cutoff = get_length_at(n) + offset;
    uint8_t* data = generate_data(cutoff);
    GWBUF* buffer = create_test_buffer();
    int len = gwbuf_length(buffer);
    uint8_t dest[cutoff];

    memset(dest, 0, sizeof(dest));
    ss_info_dassert(gwbuf_copy_data(buffer, 0, cutoff, dest) == cutoff, "All bytes should be read");
    ss_info_dassert(memcmp(data, dest, sizeof(dest)) == 0, "Data should be OK");
    gwbuf_free(buffer);
}

/** gwbuf_split test - These tests assume allocation will always succeed */
void test_split()
{
    size_t headsize = 10;
    GWBUF* head = gwbuf_alloc(headsize);
    size_t tailsize = 20;
    GWBUF* tail = gwbuf_alloc(tailsize);

    GWBUF* oldchain = gwbuf_append(gwbuf_alloc(headsize), gwbuf_alloc(tailsize));
    ss_info_dassert(gwbuf_length(oldchain) == headsize + tailsize, "Allocated buffer should be 30 bytes");
    GWBUF* newchain = gwbuf_split(&oldchain, headsize + 5);
    ss_info_dassert(newchain && oldchain, "Both chains should be non-NULL");
    ss_info_dassert(gwbuf_length(newchain) == headsize + 5, "New chain should be 15 bytes long");
    ss_info_dassert(GWBUF_LENGTH(newchain) == headsize && GWBUF_LENGTH(newchain->next) == 5,
                    "The new chain should have a 10 byte buffer and a 5 byte buffer");
    ss_info_dassert(gwbuf_length(oldchain) == tailsize - 5, "Old chain should be 15 bytes long");
    ss_info_dassert(GWBUF_LENGTH(oldchain) == tailsize - 5 && oldchain->next == NULL,
                    "The old chain should have a 15 byte buffer and no next buffer");
    gwbuf_free(oldchain);
    gwbuf_free(newchain);

    oldchain = gwbuf_append(gwbuf_alloc(headsize), gwbuf_alloc(tailsize));
    newchain = gwbuf_split(&oldchain, headsize);
    ss_info_dassert(gwbuf_length(newchain) == headsize, "New chain should be 10 bytes long");
    ss_info_dassert(gwbuf_length(oldchain) == tailsize, "Old chain should be 20 bytes long");
    ss_info_dassert(oldchain->tail == oldchain, "Old chain tail should point to old chain");
    ss_info_dassert(oldchain->next == NULL, "Old chain should not have next buffer");
    ss_info_dassert(newchain->tail == newchain, "Old chain tail should point to old chain");
    ss_info_dassert(newchain->next == NULL, "new chain should not have next buffer");
    gwbuf_free(oldchain);
    gwbuf_free(newchain);

    oldchain = gwbuf_append(gwbuf_alloc(headsize), gwbuf_alloc(tailsize));
    newchain = gwbuf_split(&oldchain, headsize + tailsize);
    ss_info_dassert(newchain, "New chain should be non-NULL");
    ss_info_dassert(gwbuf_length(newchain) == headsize + tailsize, "New chain should be 30 bytes long");
    ss_info_dassert(oldchain == NULL, "Old chain should be NULL");

    /** Splitting of contiguous memory */
    GWBUF* buffer = gwbuf_alloc(10);
    GWBUF* newbuf = gwbuf_split(&buffer, 5);
    ss_info_dassert(buffer != newbuf, "gwbuf_split should return different pointers");
    ss_info_dassert(gwbuf_length(buffer) == 5 && GWBUF_LENGTH(buffer) == 5, "Old buffer should be 5 bytes");
    ss_info_dassert(gwbuf_length(newbuf) == 5 && GWBUF_LENGTH(newbuf) == 5, "New buffer should be 5 bytes");
    ss_info_dassert(buffer->tail == buffer, "Old buffer's tail should point to itself");
    ss_info_dassert(newbuf->tail == newbuf, "New buffer's tail should point to itself");
    ss_info_dassert(buffer->next == NULL, "Old buffer's next pointer should be NULL");
    ss_info_dassert(newbuf->next == NULL, "New buffer's next pointer should be NULL");

    /** Bad parameter tests */
    GWBUF* ptr = NULL;
    ss_info_dassert(gwbuf_split(NULL, 0) == NULL, "gwbuf_split with NULL parameter should return NULL");
    ss_info_dassert(gwbuf_split(&ptr, 0) == NULL, "gwbuf_split with pointer to a NULL value should return NULL");
    buffer = gwbuf_alloc(10);
    ss_info_dassert(gwbuf_split(&buffer, 0) == NULL, "gwbuf_split with length of 0 should return NULL");
    ss_info_dassert(gwbuf_length(buffer) == 10, "Buffer should be 10 bytes");
    gwbuf_free(buffer);
    gwbuf_free(newbuf);

    /** Splitting near buffer boudaries */
    for (int i = 0; i < n_buffers - 1; i++)
    {
        split_buffer(i, -1);
        split_buffer(i, 0);
        split_buffer(i, 1);
    }

    /** Split near last buffer's end */
    split_buffer(n_buffers - 1, -1);
}

/** gwbuf_alloc_and_load and gwbuf_copy_data tests */
void test_load_and_copy()
{
    uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint8_t dest[8];
    GWBUF* head = gwbuf_alloc_and_load(4, data);
    GWBUF* tail = gwbuf_alloc_and_load(4, data + 4);

    ss_info_dassert(memcmp(GWBUF_DATA(head), data, 4) == 0, "Loading 4 bytes should succeed");
    ss_info_dassert(memcmp(GWBUF_DATA(tail), data + 4, 4) == 0, "Loading 4 bytes should succeed");

    memset(dest, 0, sizeof(dest));
    ss_info_dassert(gwbuf_copy_data(head, 0, 4, dest) == 4, "Copying 4 bytes should succeed");
    ss_info_dassert(memcmp(dest, data, 4) == 0, "Copied data should be from 1 to 4");

    memset(dest, 0, sizeof(dest));
    ss_info_dassert(gwbuf_copy_data(tail, 0, 4, dest) == 4, "Copying 4 bytes should succeed");
    ss_info_dassert(memcmp(dest, data + 4, 4) == 0, "Copied data should be from 5 to 8");
    head = gwbuf_append(head, tail);

    memset(dest, 0, sizeof(dest));
    ss_info_dassert(gwbuf_copy_data(head, 0, 8, dest) == 8, "Copying 8 bytes should succeed");
    ss_info_dassert(memcmp(dest, data, 8) == 0, "Copied data should be from 1 to 8");

    memset(dest, 0, sizeof(dest));
    ss_info_dassert(gwbuf_copy_data(head, 4, 4, dest) == 4, "Copying 4 bytes at offset 4 should succeed");
    ss_info_dassert(memcmp(dest, data + 4, 4) == 0, "Copied data should be from 5 to 8");

    memset(dest, 0, sizeof(dest));
    ss_info_dassert(gwbuf_copy_data(head, 2, 4, dest) == 4, "Copying 4 bytes at offset 2 should succeed");
    ss_info_dassert(memcmp(dest, data + 2, 4) == 0, "Copied data should be from 5 to 8");

    memset(dest, 0, sizeof(dest));
    ss_info_dassert(gwbuf_copy_data(head, 0, 10, dest) == 8, "Copying 10 bytes should only copy 8 bytes");
    ss_info_dassert(memcmp(dest, data, 8) == 0, "Copied data should be from 1 to 8");

    memset(dest, 0, sizeof(dest));
    ss_info_dassert(gwbuf_copy_data(head, 0, 0, dest) == 0, "Copying 0 bytes should not copy any bytes");

    memset(dest, 0, sizeof(dest));
    ss_info_dassert(gwbuf_copy_data(head, 0, -1, dest) == sizeof(data),
                    "Copying -1 bytes should copy all available data (cast to unsigned)");
    ss_info_dassert(memcmp(dest, data, 8) == 0, "Copied data should be from 1 to 8");

    ss_info_dassert(gwbuf_copy_data(head, -1, -1, dest) == 0,
                    "Copying -1 bytes at an offset of -1 should not copy any bytes");
    ss_info_dassert(gwbuf_copy_data(head, -1, 0, dest) == 0,
                    "Copying 0 bytes at an offset of -1 should not copy any bytes");
    gwbuf_free(head);

    /** Copying near buffer boudaries */
    for (int i = 0; i < n_buffers - 1; i++)
    {
        copy_buffer(i, -1);
        copy_buffer(i, 0);
        copy_buffer(i, 1);
    }

    /** Copy near last buffer's end */
    copy_buffer(n_buffers - 1, -1);
}

void test_consume()
{
    uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    GWBUF* buffer = gwbuf_append(gwbuf_alloc_and_load(5, data),
                                 gwbuf_alloc_and_load(5, data + 5));

    ss_info_dassert(gwbuf_consume(buffer, 0) == buffer,
                    "Consuming 0 bytes from a buffer should return original buffer");
    ss_info_dassert(gwbuf_length(buffer) == 10, "Buffer should be 10 bytes after consuming 0 bytes");

    buffer = gwbuf_consume(buffer, 1);
    ss_info_dassert(GWBUF_LENGTH(buffer) == 4, "First buffer should be 4 bytes long");
    ss_info_dassert(buffer->next, "Buffer should have next pointer set");
    ss_info_dassert(GWBUF_LENGTH(buffer->next) == 5, "Next buffer should be 5 bytes long");
    ss_info_dassert(gwbuf_length(buffer) == 9, "Buffer should be 9 bytes after consuming 1 bytes");
    ss_info_dassert(*((uint8_t*)buffer->start) == 2, "First byte should be 2");

    buffer = gwbuf_consume(buffer, 5);
    ss_info_dassert(buffer->next == NULL, "Buffer should not have the next pointer set");
    ss_info_dassert(GWBUF_LENGTH(buffer) == 4, "Buffer should be 4 bytes after consuming 6 bytes");
    ss_info_dassert(gwbuf_length(buffer) == 4, "Buffer should be 4 bytes after consuming 6 bytes");
    ss_info_dassert(*((uint8_t*)buffer->start) == 7, "First byte should be 7");
    ss_info_dassert(gwbuf_consume(buffer, 4) == NULL, "Consuming all bytes should return NULL");

    buffer = gwbuf_append(gwbuf_alloc_and_load(5, data),
                          gwbuf_alloc_and_load(5, data + 5));
    ss_info_dassert(gwbuf_consume(buffer, 100) == NULL,
                    "Consuming more bytes than are available should return NULL");


    /** Consuming near buffer boudaries */
    for (int i = 0; i < n_buffers - 1; i++)
    {
        consume_buffer(i, -1);
        consume_buffer(i, 0);
        consume_buffer(i, 1);
    }

    /** Consume near last buffer's end */
    consume_buffer(n_buffers - 1, -1);
}

/**
 * test1    Allocate a buffer and do lots of things
 *
 */
static int
test1()
{
    GWBUF   *buffer, *extra, *clone, *partclone, *transform;
    HINT    *hint;
    int     size = 100;
    int     bite1 = 35;
    int     bite2 = 60;
    int     bite3 = 10;
    int     buflen;

    /* Single buffer tests */
    ss_dfprintf(stderr,
                "testbuffer : creating buffer with data size %d bytes",
                size);
    buffer = gwbuf_alloc(size);
    ss_dfprintf(stderr, "\t..done\nAllocated buffer of size %d.", size);
    buflen = GWBUF_LENGTH(buffer);
    ss_dfprintf(stderr, "\nBuffer length is now %d", buflen);
    ss_info_dassert(size == buflen, "Incorrect buffer size");
    ss_info_dassert(0 == GWBUF_EMPTY(buffer), "Buffer should not be empty");
    ss_info_dassert(GWBUF_IS_TYPE_UNDEFINED(buffer), "Buffer type should be undefined");
    ss_dfprintf(stderr, "\t..done\nSet a hint for the buffer");
    hint = hint_create_parameter(NULL, "name", "value");
    gwbuf_add_hint(buffer, hint);
    ss_info_dassert(hint == buffer->hint, "Buffer should point to first and only hint");
    ss_dfprintf(stderr, "\t..done\nSet a property for the buffer");
    gwbuf_add_property(buffer, "name", "value");
    ss_info_dassert(0 == strcmp("value", gwbuf_get_property(buffer, "name")), "Should now have correct property");
    strcpy(GWBUF_DATA(buffer), "The quick brown fox jumps over the lazy dog");
    ss_dfprintf(stderr, "\t..done\nLoad some data into the buffer");
    ss_info_dassert('q' == GWBUF_DATA_CHAR(buffer, 4), "Fourth character of buffer must be 'q'");
    ss_info_dassert(-1 == GWBUF_DATA_CHAR(buffer, 105), "Hundred and fifth character of buffer must return -1");
    ss_info_dassert(0 == GWBUF_IS_SQL(buffer), "Must say buffer is not SQL, as it does not have marker");
    strcpy(GWBUF_DATA(buffer), "1234\x03SELECT * FROM sometable");
    ss_dfprintf(stderr, "\t..done\nLoad SQL data into the buffer");
    ss_info_dassert(1 == GWBUF_IS_SQL(buffer), "Must say buffer is SQL, as it does have marker");
    transform = gwbuf_clone_transform(buffer, GWBUF_TYPE_PLAINSQL);
    ss_dfprintf(stderr, "\t..done\nAttempt to transform buffer to plain SQL - should fail");
    ss_info_dassert(NULL == transform, "Buffer cannot be transformed to plain SQL");
    gwbuf_set_type(buffer, GWBUF_TYPE_MYSQL);
    ss_dfprintf(stderr, "\t..done\nChanged buffer type to MySQL");
    ss_info_dassert(GWBUF_IS_TYPE_MYSQL(buffer), "Buffer type changed to MySQL");
    transform = gwbuf_clone_transform(buffer, GWBUF_TYPE_PLAINSQL);
    ss_dfprintf(stderr, "\t..done\nAttempt to transform buffer to plain SQL - should succeed");
    ss_info_dassert((NULL != transform) &&
                    (GWBUF_IS_TYPE_PLAINSQL(transform)), "Transformed buffer is plain SQL");
    clone = gwbuf_clone(buffer);
    ss_dfprintf(stderr, "\t..done\nCloned buffer");
    buflen = GWBUF_LENGTH(clone);
    ss_dfprintf(stderr, "\nCloned buffer length is now %d", buflen);
    ss_info_dassert(size == buflen, "Incorrect buffer size");
    ss_info_dassert(0 == GWBUF_EMPTY(clone), "Cloned buffer should not be empty");
    ss_dfprintf(stderr, "\t..done\n");
    gwbuf_free(clone);
    ss_dfprintf(stderr, "Freed cloned buffer");
    ss_dfprintf(stderr, "\t..done\n");
    partclone = gwbuf_clone_portion(buffer, 25, 50);
    buflen = GWBUF_LENGTH(partclone);
    ss_dfprintf(stderr, "Part cloned buffer length is now %d", buflen);
    ss_info_dassert(50 == buflen, "Incorrect buffer size");
    ss_info_dassert(0 == GWBUF_EMPTY(partclone), "Part cloned buffer should not be empty");
    ss_dfprintf(stderr, "\t..done\n");
    gwbuf_free(partclone);
    ss_dfprintf(stderr, "Freed part cloned buffer");
    ss_dfprintf(stderr, "\t..done\n");
    buffer = gwbuf_consume(buffer, bite1);
    ss_info_dassert(NULL != buffer, "Buffer should not be null");
    buflen = GWBUF_LENGTH(buffer);
    ss_dfprintf(stderr, "Consumed %d bytes, now have %d, should have %d", bite1, buflen, size - bite1);
    ss_info_dassert((size - bite1) == buflen, "Incorrect buffer size");
    ss_info_dassert(0 == GWBUF_EMPTY(buffer), "Buffer should not be empty");
    ss_dfprintf(stderr, "\t..done\n");
    buffer = gwbuf_consume(buffer, bite2);
    ss_info_dassert(NULL != buffer, "Buffer should not be null");
    buflen = GWBUF_LENGTH(buffer);
    ss_dfprintf(stderr, "Consumed %d bytes, now have %d, should have %d", bite2, buflen, size - bite1 - bite2);
    ss_info_dassert((size - bite1 - bite2) == buflen, "Incorrect buffer size");
    ss_info_dassert(0 == GWBUF_EMPTY(buffer), "Buffer should not be empty");
    ss_dfprintf(stderr, "\t..done\n");
    buffer = gwbuf_consume(buffer, bite3);
    ss_dfprintf(stderr, "Consumed %d bytes, should have null buffer", bite3);
    ss_info_dassert(NULL == buffer, "Buffer should be null");

    /* Buffer list tests */
    size = 100000;
    buffer = gwbuf_alloc(size);
    ss_dfprintf(stderr, "\t..done\nAllocated buffer of size %d.", size);
    buflen = GWBUF_LENGTH(buffer);
    ss_dfprintf(stderr, "\nBuffer length is now %d", buflen);
    ss_info_dassert(size == buflen, "Incorrect buffer size");
    ss_info_dassert(0 == GWBUF_EMPTY(buffer), "Buffer should not be empty");
    ss_info_dassert(GWBUF_IS_TYPE_UNDEFINED(buffer), "Buffer type should be undefined");
    extra = gwbuf_alloc(size);
    buflen = GWBUF_LENGTH(buffer);
    ss_dfprintf(stderr, "\t..done\nAllocated extra buffer of size %d.", size);
    ss_info_dassert(size == buflen, "Incorrect buffer size");
    buffer = gwbuf_append(buffer, extra);
    buflen = gwbuf_length(buffer);
    ss_dfprintf(stderr, "\t..done\nAppended extra buffer to original buffer to create list of size %d", buflen);
    ss_info_dassert((size * 2) == gwbuf_length(buffer), "Incorrect size for set of buffers");
    buffer = gwbuf_rtrim(buffer, 60000);
    buflen = GWBUF_LENGTH(buffer);
    ss_dfprintf(stderr, "\t..done\nTrimmed 60 bytes from buffer, now size is %d.", buflen);
    ss_info_dassert((size - 60000) == buflen, "Incorrect buffer size");
    buffer = gwbuf_rtrim(buffer, 60000);
    buflen = GWBUF_LENGTH(buffer);
    ss_dfprintf(stderr, "\t..done\nTrimmed another 60 bytes from buffer, now size is %d.", buflen);
    ss_info_dassert(100000 == buflen, "Incorrect buffer size");
    ss_info_dassert(buffer == extra, "The buffer pointer should now point to the extra buffer");
    ss_dfprintf(stderr, "\t..done\n");

    /** gwbuf_clone_all test  */
    size_t headsize = 10;
    GWBUF* head = gwbuf_alloc(headsize);
    size_t tailsize = 20;
    GWBUF* tail = gwbuf_alloc(tailsize);

    ss_info_dassert(head && tail, "Head and tail buffers should both be non-NULL");
    GWBUF* append = gwbuf_append(head, tail);
    ss_info_dassert(append == head, "gwbuf_append should return head");
    ss_info_dassert(append->next == tail, "After append tail should be in the next pointer of head");
    ss_info_dassert(append->tail == tail, "After append tail should be in the tail pointer of head");
    GWBUF* all_clones = gwbuf_clone_all(head);
    ss_info_dassert(all_clones && all_clones->next, "Cloning all should work");
    ss_info_dassert(GWBUF_LENGTH(all_clones) == headsize, "First buffer should be 10 bytes");
    ss_info_dassert(GWBUF_LENGTH(all_clones->next) == tailsize, "Second buffer should be 20 bytes");
    ss_info_dassert(gwbuf_length(all_clones) == headsize + tailsize, "Total buffer length should be 30 bytes");

    test_split();
    test_load_and_copy();
    test_consume();

    return 0;
}

int main(int argc, char **argv)
{
    int result = 0;

    result += test1();

    exit(result);
}


