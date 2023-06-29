#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <uctest/minunit.h>
#include <testdata.h>
#include <ucrypt/sha.h>

/*
 * Test SHA-256 implementation based on test cases defined in NIST's SHA examples document
 * https://csrc.nist.gov/csrc/media/projects/cryptographic-standards-and-guidelines/documents/examples/sha_all.pdf
 */

#define NOF_TESTCASES   6

struct sha256_testcase {
    const char *message;
    const uint64_t message_length; /* message length in bytes */
    const char *digest;
    uint64_t repetitions;
} hashes[NOF_TESTCASES] = {
        {
            "abc",
            3,
            "\xba\x78\x16\xbf\x8f\x01\xcf\xea\x41\x41\x40\xde\x5d\xae\x22\x23\xb0\x03\x61\xa3\x96\x17\x7a\x9c\xb4\x10\xff\x61\xf2\x00\x15\xad",
            1
        },
        {
            "",
            0,
            "\xe3\xb0\xc4\x42\x98\xfc\x1c\x14\x9a\xfb\xf4\xc8\x99\x6f\xb9\x24\x27\xae\x41\xe4\x64\x9b\x93\x4c\xa4\x95\x99\x1b\x78\x52\xb8\x55",
            1
        },
        {
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
            56,
            "\x24\x8d\x6a\x61\xd2\x06\x38\xb8\xe5\xc0\x26\x93\x0c\x3e\x60\x39\xa3\x3c\xe4\x59\x64\xff\x21\x67\xf6\xec\xed\xd4\x19\xdb\x06\xc1",
            1
        },
        {
            "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
            112,
            "\xcf\x5b\x16\xa7\x78\xaf\x83\x80\x03\x6c\xe5\x9e\x7b\x04\x92\x37\x0b\x24\x9b\x11\xe8\xf0\x7a\x51\xaf\xac\x45\x03\x7a\xfe\xe9\xd1",
            1
        },
        {
            "a",
            1,
            "\xcd\xc7\x6e\x5c\x99\x14\xfb\x92\x81\xa1\xc7\xe2\x84\xd7\x3e\x67\xf1\x80\x9a\x48\xa4\x97\x20\x0e\x04\x6d\x39\xcc\xc7\x11\x2c\xd0",
            1000000
        },
        {
            "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno",
            64,
            "\x50\xe7\x2a\x0e\x26\x44\x2f\xe2\x55\x2d\xc3\x93\x8a\xc5\x86\x58\x22\x8c\x0c\xbf\xb1\xd2\xca\x87\x2a\xe4\x35\x26\x6f\xcd\x05\x5e",
            16777216
        }
};

int is_equal(uint8_t *digest_baseline, uint8_t *digest)
{
    int i, equal;

    equal = 1;
    for ( i = 0; i < UC_SHA256_DIGEST_SIZE; ++i )
    {
        if ( digest_baseline[i] != digest[i] )
        {
            equal = 0;
            break;
        }
    }

    return equal;
}

void print_digest(uint8_t *digest)
{
    int i;

    for ( i = 0; i < UC_SHA256_DIGEST_SIZE; ++i )
        printf("%02x", digest[i]);
    puts("");
}

char *test_sha256()
{
    int i, success;
    uint64_t j;
    uint8_t digest[UC_SHA256_DIGEST_SIZE];
    uint8_t *tc_input;
    uint8_t tc_digest[UC_SHA256_DIGEST_SIZE];

    puts("");
    puts("Running SHA-256 testcases...");

    for ( i = 0; i < NOF_TESTCASES; ++i )
    {
        printf("Running testcase %i...\n", i);

        uc_sha_256_ctx_t ctx;
        uc_sha256_init(&ctx);

        tc_input = malloc(hashes[i].message_length);
        for ( j = 0; j < hashes[i].message_length; ++j )
            tc_input[j] = (uint8_t) hashes[i].message[j];
        for ( j = 0; j < UC_SHA256_DIGEST_SIZE; ++j )
            tc_digest[j] = (uint8_t) hashes[i].digest[j];

        for ( j = 0; j < hashes[i].repetitions; ++j )
            uc_sha256_update(&ctx, tc_input, hashes[i].message_length);
        uc_sha256_finalize(&ctx);
        uc_sha256_output(&ctx, digest);
//        print_digest(digest);
//        print_digest(tc_digest);

        success = is_equal(tc_digest, digest);
        mu_assert("Testcase failed", success);

        free(tc_input);
    }

    return NULL;
}
