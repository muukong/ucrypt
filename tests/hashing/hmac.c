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

#define NOF_TESTCASES   5

struct hmac_sha256_testcase {
    const char *key;
    const uint64_t key_length;
    uint8_t *data;
    const uint64_t data_length;
    const char *tag;
} hmac_sha256_testcases[NOF_TESTCASES] = {
        {   /* Test Case 1 (https://datatracker.ietf.org/doc/html/rfc4231#section-4.2) */
                "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b",
                20,
                "\x48\x69\x20\x54\x68\x65\x72\x65",
                8,
                "\xb0\x34\x4c\x61\xd8\xdb\x38\x53\x5c\xa8\xaf\xce\xaf\x0b\xf1\x2b\x88\x1d\xc2\x00\xc9\x83\x3d\xa7\x26\xe9\x37\x6c\x2e\x32\xcf\xf7"
        },
        {   /* Test Case 2 (https://datatracker.ietf.org/doc/html/rfc4231#section-4.3) */
                "\x4a\x65\x66\x65",
                4,
                "\x77\x68\x61\x74\x20\x64\x6f\x20\x79\x61\x20\x77\x61\x6e\x74\x20\x66\x6f\x72\x20\x6e\x6f\x74\x68\x69\x6e\x67\x3f",
                28,
                "\x5b\xdc\xc1\x46\xbf\x60\x75\x4e\x6a\x04\x24\x26\x08\x95\x75\xc7\x5a\x00\x3f\x08\x9d\x27\x39\x83\x9d\xec\x58\xb9\x64\xec\x38\x43"
        },
        {   /* Test Case 3 (https://datatracker.ietf.org/doc/html/rfc4231#section-4.4) */
                "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa",
                20,
                "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd",
                50,
                "\x77\x3e\xa9\x1e\x36\x80\x0e\x46\x85\x4d\xb8\xeb\xd0\x91\x81\xa7\x29\x59\x09\x8b\x3e\xf8\xc1\x22\xd9\x63\x55\x14\xce\xd5\x65\xfe"
        },
        {   /* Test Case 4 (https://datatracker.ietf.org/doc/html/rfc4231#section-4.4) */
                "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19",
                25,
                "\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd",
                50,
                "\x82\x55\x8a\x38\x9a\x44\x3c\x0e\xa4\xcc\x81\x98\x99\xf2\x08\x3a\x85\xf0\xfa\xa3\xe5\x78\xf8\x07\x7a\x2e\x3f\xf4\x67\x29\x66\x5b"
        },
        /* Test Case 5 is omitted as truncation is currently not implemented */
        {   /* Test Case 6 (https://datatracker.ietf.org/doc/html/rfc4231#section-4.7) */
                "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa",
                131,
                "\x54\x65\x73\x74\x20\x55\x73\x69\x6e\x67\x20\x4c\x61\x72\x67\x65\x72\x20\x54\x68\x61\x6e\x20\x42\x6c\x6f\x63\x6b\x2d\x53\x69\x7a\x65\x20\x4b\x65\x79\x20\x2d\x20\x48\x61\x73\x68\x20\x4b\x65\x79\x20\x46\x69\x72\x73\x74",
                54,
                "\x60\xe4\x31\x59\x1e\xe0\xb6\x7f\x0d\x8a\x26\xaa\xcb\xf5\xb7\x7f\x8e\x0b\xc6\x21\x37\x28\xc5\x14\x05\x46\x04\x0f\x0e\xe3\x7f\x54"
        }

};

static int is_equal(uint8_t *digest_baseline, uint8_t *digest)
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

static void print_digest(uint8_t *digest)
{
    int i;

    for ( i = 0; i < UC_SHA256_DIGEST_SIZE; ++i )
        printf("%02x", digest[i]);
    puts("");
}

char *test_hmac()
{
    int success;
    uint64_t i, j;
    uint8_t *key, *data;
    uint8_t tc_tag[UC_SHA256_DIGEST_SIZE];
    uint8_t tag[UC_SHA256_DIGEST_SIZE];

    printf("[*] Running HMAC testcases...\n");

    for ( i = 0; i < NOF_TESTCASES; ++i )
    {
        key = malloc(hmac_sha256_testcases[i].key_length);
        for ( j = 0; j < hmac_sha256_testcases[i].key_length; ++j)
            key[j] = (uint8_t) hmac_sha256_testcases[i].key[j];

        data = malloc(hmac_sha256_testcases[i].data_length);
        for ( j = 0; j < hmac_sha256_testcases[i].data_length; ++j )
            data[j] = (uint8_t) hmac_sha256_testcases[i].data[j];

        for ( j = 0; j < UC_SHA256_DIGEST_SIZE; ++j )
            tc_tag[j] = hmac_sha256_testcases[i].tag[j];

        uc_sha_hmac_ctx_t hmac;
        uc_sha_hmac_init(&hmac, UC_SHA256, key, hmac_sha256_testcases[i].key_length);
        uc_sha_hmac_update(&hmac, data, hmac_sha256_testcases[i].data_length);
        uc_sha_hmac_finalize(&hmac);
        uc_sha_hmac_output(&hmac, tag);

        /* Check if calculated tag matches test case tag */
        success = is_equal(tc_tag, tag);
        mu_assert("HMAC testcase failed", success);

        free(key);
        free(data);
    }

    return NULL;
}
