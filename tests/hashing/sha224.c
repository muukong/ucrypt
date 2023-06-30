#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <uctest/minunit.h>
#include <testdata.h>
#include <ucrypt/sha.h>

/*
 * Test SHA-224 unit tests based on test cases defined in NIST's SHA examples document
 * https://csrc.nist.gov/csrc/media/projects/cryptographic-standards-and-guidelines/documents/examples/sha_all.pdf
 */

#define NOF_TESTCASES   6

struct sha224_testcase {
    const char *message;
    const uint64_t message_length; /* message length in bytes */
    const char *digest;
    uint64_t repetitions;
} sha224_hashes[NOF_TESTCASES] = {
    {
        "abc",
        3,
        "\x23\x09\x7d\x22\x34\x05\xd8\x22\x86\x42\xa4\x77\xbd\xa2\x55\xb3\x2a\xad\xbc\xe4\xbd\xa0\xb3\xf7\xe3\x6c\x9d\xa7",
        1
    },
    {
        "",
        0,
        "\xd1\x4a\x02\x8c\x2a\x3a\x2b\xc9\x47\x61\x02\xbb\x28\x82\x34\xc4\x15\xa2\xb0\x1f\x82\x8e\xa6\x2a\xc5\xb3\xe4\x2f",
        1
    },
    {
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
        56,
        "\x75\x38\x8b\x16\x51\x27\x76\xcc\x5d\xba\x5d\xa1\xfd\x89\x01\x50\xb0\xc6\x45\x5c\xb4\xf5\x8b\x19\x52\x52\x25\x25",
        1
    },
    {
        "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
        112,
        "\xc9\x7c\xa9\xa5\x59\x85\x0c\xe9\x7a\x04\xa9\x6d\xef\x6d\x99\xa9\xe0\xe0\xe2\xab\x14\xe6\xb8\xdf\x26\x5f\xc0\xb3",
        1
    },
    {
        "a",
        1,
        "\x20\x79\x46\x55\x98\x0c\x91\xd8\xbb\xb4\xc1\xea\x97\x61\x8a\x4b\xf0\x3f\x42\x58\x19\x48\xb2\xee\x4e\xe7\xad\x67",
        1000000
    },
    {
        "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno",
        64,
        "\xb5\x98\x97\x13\xca\x4f\xe4\x7a\x00\x9f\x86\x21\x98\x0b\x34\xe6\xd6\x3e\xd3\x06\x3b\x2a\x0a\x2c\x86\x7d\x8a\x85",
        16777216
    }
};

static int is_equal(uint8_t *digest_baseline, uint8_t *digest)
{
    int i, equal;

    equal = 1;
    for ( i = 0; i < UC_SHA224_DIGEST_SIZE; ++i )
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

    for ( i = 0; i < UC_SHA224_DIGEST_SIZE; ++i )
        printf("%02x", digest[i]);
    puts("");
}

char *test_sha224()
{
    int i, success;
    uint64_t j;
    uint8_t digest[UC_SHA224_DIGEST_SIZE];
    uint8_t *tc_input;
    uint8_t tc_digest[UC_SHA224_DIGEST_SIZE];

    puts("[*] Running SHA-224 testcases...");

    for ( i = 0; i < NOF_TESTCASES; ++i )
    {
        printf("Running testcase %i...\n", i);

        uc_sha_224_ctx_t ctx;
        uc_sha224_init(&ctx);

        tc_input = malloc(sha224_hashes[i].message_length);
        for ( j = 0; j < sha224_hashes[i].message_length; ++j )
            tc_input[j] = (uint8_t) sha224_hashes[i].message[j];
        for ( j = 0; j < UC_SHA224_DIGEST_SIZE; ++j )
            tc_digest[j] = (uint8_t) sha224_hashes[i].digest[j];

        for ( j = 0; j < sha224_hashes[i].repetitions; ++j )
            uc_sha224_update(&ctx, tc_input, sha224_hashes[i].message_length);
        uc_sha224_finalize(&ctx);
        uc_sha224_output(&ctx, digest);
        success = is_equal(tc_digest, digest);
        mu_assert("SHA-224 testcase failed", success);

        free(tc_input);
    }

    return NULL;
}
