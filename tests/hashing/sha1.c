#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <uctest/minunit.h>
#include <testdata.h>
#include <ucrypt/sha.h>

/*
 * Test SHA-1 unit tests based on test cases defined in NIST's SHA examples document
 * https://csrc.nist.gov/csrc/media/projects/cryptographic-standards-and-guidelines/documents/examples/sha_all.pdf
 */

#define NOF_TESTCASES   6

struct sha1_testcase {
    const char *message;
    const uint64_t message_length; /* message length in bytes */
    const char *digest;
    uint64_t repetitions;
} sha1_hashes[NOF_TESTCASES] = {
    {
        "abc",
        3,
        "\xa9\x99\x3e\x36\x47\x06\x81\x6a\xba\x3e\x25\x71\x78\x50\xc2\x6c\x9c\xd0\xd8\x9d",
        1
    },
    {
        "",
        0,
        "\xda\x39\xa3\xee\x5e\x6b\x4b\x0d\x32\x55\xbf\xef\x95\x60\x18\x90\xaf\xd8\x07\x09",
        1
    },
    {
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
        56,
        "\x84\x98\x3e\x44\x1c\x3b\xd2\x6e\xba\xae\x4a\xa1\xf9\x51\x29\xe5\xe5\x46\x70\xf1",
        1
    },
    {
        "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
        112,
        "\xa4\x9b\x24\x46\xa0\x2c\x64\x5b\xf4\x19\xf9\x95\xb6\x70\x91\x25\x3a\x04\xa2\x59",
        1
    },
    {
        "a",
        1,
        "\x34\xaa\x97\x3c\xd4\xc4\xda\xa4\xf6\x1e\xeb\x2b\xdb\xad\x27\x31\x65\x34\x01\x6f",
        1000000
    },
    {
        "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno",
        64,
        "\x77\x89\xf0\xc9\xef\x7b\xfc\x40\xd9\x33\x11\x14\x3d\xfb\xe6\x9e\x20\x17\xf5\x92",
        16777216
    }
};

static int is_equal(uint8_t *digest_baseline, uint8_t *digest)
{
    int i, equal;

    equal = 1;
    for ( i = 0; i < UC_SHA1_DIGEST_SIZE; ++i )
    {
        if ( digest_baseline[i] != digest[i] )
        {
            equal = 0;
            break;
        }
    }

    return equal;
}

static void print_digest(uint8_t *digest)
{
    int i;

    for ( i = 0; i < UC_SHA1_DIGEST_SIZE; ++i )
        printf("%02x", digest[i]);
    puts("");
}

char *test_sha1()
{
    int i, success;
    uint64_t j;
    uint8_t digest[UC_SHA1_DIGEST_SIZE];
    uint8_t *tc_input;
    uint8_t tc_digest[UC_SHA1_DIGEST_SIZE];

    puts("[*] Running SHA-1 testcases...");

    for ( i = 0; i < NOF_TESTCASES; ++i )
    {
        printf("Running testcase %i...\n", i);

        uc_sha_1_ctx_t ctx;
        uc_sha1_init(&ctx);

        tc_input = malloc(sha1_hashes[i].message_length);
        for ( j = 0; j < sha1_hashes[i].message_length; ++j )
            tc_input[j] = (uint8_t) sha1_hashes[i].message[j];
        for ( j = 0; j < UC_SHA1_DIGEST_SIZE; ++j )
            tc_digest[j] = (uint8_t) sha1_hashes[i].digest[j];

        for ( j = 0; j < sha1_hashes[i].repetitions; ++j )
            uc_sha1_update(&ctx, tc_input, sha1_hashes[i].message_length);
        uc_sha1_finalize(&ctx);
        uc_sha1_output(&ctx, digest);
        success = is_equal(tc_digest, digest);
        mu_assert("SHA-1 testcase failed", success);

        free(tc_input);
    }

    return NULL;
}
