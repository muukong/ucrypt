#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <uctest/minunit.h>
#include <testdata.h>
#include <ucrypt/sha.h>

/*
 * Test SHA-512 unit tests based on test cases defined in NIST's SHA examples document
 * https://csrc.nist.gov/csrc/media/projects/cryptographic-standards-and-guidelines/documents/examples/sha_all.pdf
 */

#define NOF_TESTCASES   6

static struct sha512_testcase {
    const char *message;
    const uint64_t message_length; /* message length in bytes */
    const char *digest;
    uint64_t repetitions;
} sha512_hashes[NOF_TESTCASES] = {
        { /* testcase 0 */
            "abc",
            3,
            "\xdd\xaf\x35\xa1\x93\x61\x7a\xba\xcc\x41\x73\x49\xae\x20\x41\x31\x12\xe6\xfa\x4e\x89\xa9\x7e\xa2\x0a\x9e\xee\xe6\x4b\x55\xd3\x9a\x21\x92\x99\x2a\x27\x4f\xc1\xa8\x36\xba\x3c\x23\xa3\xfe\xeb\xbd\x45\x4d\x44\x23\x64\x3c\xe8\x0e\x2a\x9a\xc9\x4f\xa5\x4c\xa4\x9f",
            1
        },
        { /* testcase 1 */
            "",
            0,
            "\xcf\x83\xe1\x35\x7e\xef\xb8\xbd\xf1\x54\x28\x50\xd6\x6d\x80\x07\xd6\x20\xe4\x05\x0b\x57\x15\xdc\x83\xf4\xa9\x21\xd3\x6c\xe9\xce\x47\xd0\xd1\x3c\x5d\x85\xf2\xb0\xff\x83\x18\xd2\x87\x7e\xec\x2f\x63\xb9\x31\xbd\x47\x41\x7a\x81\xa5\x38\x32\x7a\xf9\x27\xda\x3e",
            1
        },
        { /* testcase 2 */
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
            56,
            "\x20\x4a\x8f\xc6\xdd\xa8\x2f\x0a\x0c\xed\x7b\xeb\x8e\x08\xa4\x16\x57\xc1\x6e\xf4\x68\xb2\x28\xa8\x27\x9b\xe3\x31\xa7\x03\xc3\x35\x96\xfd\x15\xc1\x3b\x1b\x07\xf9\xaa\x1d\x3b\xea\x57\x78\x9c\xa0\x31\xad\x85\xc7\xa7\x1d\xd7\x03\x54\xec\x63\x12\x38\xca\x34\x45",
            1
        },
        { /* testcase 4 */
            "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
            112,
            "\x8e\x95\x9b\x75\xda\xe3\x13\xda\x8c\xf4\xf7\x28\x14\xfc\x14\x3f\x8f\x77\x79\xc6\xeb\x9f\x7f\xa1\x72\x99\xae\xad\xb6\x88\x90\x18\x50\x1d\x28\x9e\x49\x00\xf7\xe4\x33\x1b\x99\xde\xc4\xb5\x43\x3a\xc7\xd3\x29\xee\xb6\xdd\x26\x54\x5e\x96\xe5\x5b\x87\x4b\xe9\x09",
            1
        },
        {
            "a",
            1,
            "\xe7\x18\x48\x3d\x0c\xe7\x69\x64\x4e\x2e\x42\xc7\xbc\x15\xb4\x63\x8e\x1f\x98\xb1\x3b\x20\x44\x28\x56\x32\xa8\x03\xaf\xa9\x73\xeb\xde\x0f\xf2\x44\x87\x7e\xa6\x0a\x4c\xb0\x43\x2c\xe5\x77\xc3\x1b\xeb\x00\x9c\x5c\x2c\x49\xaa\x2e\x4e\xad\xb2\x17\xad\x8c\xc0\x9b",
            1000000
        },
        {
            "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno",
            64,
            "\xb4\x7c\x93\x34\x21\xea\x2d\xb1\x49\xad\x6e\x10\xfc\xe6\xc7\xf9\x3d\x07\x52\x38\x01\x80\xff\xd7\xf4\x62\x9a\x71\x21\x34\x83\x1d\x77\xbe\x60\x91\xb8\x19\xed\x35\x2c\x29\x67\xa2\xe2\xd4\xfa\x50\x50\x72\x3c\x96\x30\x69\x1f\x1a\x05\xa7\x28\x1d\xbe\x6c\x10\x86",
            16777216
        }
};

static int is_equal(uint8_t *digest_baseline, uint8_t *digest)
{
    int i, equal;

    equal = 1;
    for ( i = 0; i < UC_SHA512_DIGEST_SIZE; ++i )
    {
        if ( digest_baseline[i] != digest[i] )
        {
            equal = 0;
            break;
        }
    }

    return equal;
}

char *test_sha512()
{
    int i, success;
    uint64_t j;
    uint8_t digest[UC_SHA512_DIGEST_SIZE];
    uint8_t *tc_input;
    uint8_t tc_digest[UC_SHA512_DIGEST_SIZE];

    puts("[*] Running SHA-512 testcases...");

    for ( i = 0; i < NOF_TESTCASES; ++i )
    {
        printf("Running testcase %i...\n", i);

        uc_sha_512_ctx_t ctx;
        uc_sha512_init(&ctx);

        tc_input = malloc(sha512_hashes[i].message_length);
        for ( j = 0; j < sha512_hashes[i].message_length; ++j )
            tc_input[j] = (uint8_t) sha512_hashes[i].message[j];
        for ( j = 0; j < UC_SHA512_DIGEST_SIZE; ++j )
            tc_digest[j] = (uint8_t) sha512_hashes[i].digest[j];

        for ( j = 0; j < sha512_hashes[i].repetitions; ++j )
            uc_sha512_update(&ctx, tc_input, sha512_hashes[i].message_length);
        uc_sha512_finalize(&ctx);
        uc_sha512_output(&ctx, digest);

        success = is_equal(tc_digest, digest);
        mu_assert("Testcase failed", success);

        free(tc_input);
    }

    return NULL;
}
