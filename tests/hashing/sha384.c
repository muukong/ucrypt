#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <uctest/minunit.h>
#include <testdata.h>
#include <ucrypt/sha.h>

/*
 * Test SHA-384 unit tests based on test cases defined in NIST's SHA examples document
 * https://csrc.nist.gov/csrc/media/projects/cryptographic-standards-and-guidelines/documents/examples/sha_all.pdf
 */

#define NOF_TESTCASES   6

static struct sha384_testcase {
    const char *message;
    const uint64_t message_length; /* message length in bytes */
    const char *digest;
    uint64_t repetitions;
} sha384_hashes[NOF_TESTCASES] = {
        { /* testcase 0 */
            "abc",
            3,
            "\xcb\x00\x75\x3f\x45\xa3\x5e\x8b\xb5\xa0\x3d\x69\x9a\xc6\x50\x07\x27\x2c\x32\xab\x0e\xde\xd1\x63\x1a\x8b\x60\x5a\x43\xff\x5b\xed\x80\x86\x07\x2b\xa1\xe7\xcc\x23\x58\xba\xec\xa1\x34\xc8\x25\xa7",
            1
        },
        { /* testcase 1 */
            "",
            0,
            "\x38\xb0\x60\xa7\x51\xac\x96\x38\x4c\xd9\x32\x7e\xb1\xb1\xe3\x6a\x21\xfd\xb7\x11\x14\xbe\x07\x43\x4c\x0c\xc7\xbf\x63\xf6\xe1\xda\x27\x4e\xde\xbf\xe7\x6f\x65\xfb\xd5\x1a\xd2\xf1\x48\x98\xb9\x5b",
            1
        },
        { /* testcase 2 */
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
            56,
            "\x33\x91\xfd\xdd\xfc\x8d\xc7\x39\x37\x07\xa6\x5b\x1b\x47\x09\x39\x7c\xf8\xb1\xd1\x62\xaf\x05\xab\xfe\x8f\x45\x0d\xe5\xf3\x6b\xc6\xb0\x45\x5a\x85\x20\xbc\x4e\x6f\x5f\xe9\x5b\x1f\xe3\xc8\x45\x2b",
            1
        },
        { /* testcase 4 */
            "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
            112,
            "\x09\x33\x0c\x33\xf7\x11\x47\xe8\x3d\x19\x2f\xc7\x82\xcd\x1b\x47\x53\x11\x1b\x17\x3b\x3b\x05\xd2\x2f\xa0\x80\x86\xe3\xb0\xf7\x12\xfc\xc7\xc7\x1a\x55\x7e\x2d\xb9\x66\xc3\xe9\xfa\x91\x74\x60\x39",
            1
        },
        { /* testcase 5 */
            "a",
            1,
            "\x9d\x0e\x18\x09\x71\x64\x74\xcb\x08\x6e\x83\x4e\x31\x0a\x4a\x1c\xed\x14\x9e\x9c\x00\xf2\x48\x52\x79\x72\xce\xc5\x70\x4c\x2a\x5b\x07\xb8\xb3\xdc\x38\xec\xc4\xeb\xae\x97\xdd\xd8\x7f\x3d\x89\x85",
            1000000
        },
        { /* testcase 6 */
            "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno",
            64,
            "\x54\x41\x23\x5c\xc0\x23\x53\x41\xed\x80\x6a\x64\xfb\x35\x47\x42\xb5\xe5\xc0\x2a\x3c\x5c\xb7\x1b\x5f\x63\xfb\x79\x34\x58\xd8\xfd\xae\x59\x9c\x8c\xd8\x88\x49\x43\xc0\x4f\x11\xb3\x1b\x89\xf0\x23",
            16777216
        }
};

static int is_equal(uint8_t *digest_baseline, uint8_t *digest)
{
    int i, equal;

    equal = 1;
    for ( i = 0; i < UC_SHA384_DIGEST_SIZE; ++i )
    {
        if ( digest_baseline[i] != digest[i] )
        {
            equal = 0;
            break;
        }
    }

    return equal;
}

char *test_sha384()
{
    int i, success;
    uint64_t j;
    uint8_t digest[UC_SHA384_DIGEST_SIZE];
    uint8_t *tc_input;
    uint8_t tc_digest[UC_SHA384_DIGEST_SIZE];

    puts("[*] Running SHA-384 testcases...");

    for ( i = 0; i < NOF_TESTCASES; ++i )
    {
        printf("Running testcase %i...\n", i);

        uc_sha_384_ctx_t ctx;
        uc_sha384_init(&ctx);

        tc_input = malloc(sha384_hashes[i].message_length);
        for ( j = 0; j < sha384_hashes[i].message_length; ++j )
            tc_input[j] = (uint8_t) sha384_hashes[i].message[j];
        for ( j = 0; j < UC_SHA384_DIGEST_SIZE; ++j )
            tc_digest[j] = (uint8_t) sha384_hashes[i].digest[j];

        for ( j = 0; j < sha384_hashes[i].repetitions; ++j )
            uc_sha384_update(&ctx, tc_input, sha384_hashes[i].message_length);
        uc_sha384_finalize(&ctx);
        uc_sha384_output(&ctx, digest);

        success = is_equal(tc_digest, digest);
        mu_assert("Testcase failed", success);

        free(tc_input);
    }

    return NULL;
}
