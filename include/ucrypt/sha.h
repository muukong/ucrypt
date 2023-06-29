//
// Created by urs on 6/22/23.
//

#ifndef UCRYPT_SHA_H
#define UCRYPT_SHA_H

#include <stdint.h>
#include <ucrypt/ucdef.h>
#include <ucrypt/integer.h>


#define UC_SHA256_DIGEST_SIZE 32           /* SHA-256 hash size in bytes */
#define UC_SHA256_MESSAGE_BLOCK_SIZE 64
#define UC_SHA256_MESSAGE_SCHEDULE_SIZE 64

#define UC_SHA_MAX_MESSAGE_BLOCK_SIZE UC_SHA256_MESSAGE_BLOCK_SIZE

#define UC_SHA_OK            1
#define UC_SHA_INPUT_ERROR  -1
#define UC_SHA_STATE_ERROR  -2
#define UC_SHA_NULL_ERROR   -3

typedef enum uc_sha_version_t
{
    UC_SHA256
} uc_sha_version_t;


/*
 * SHA-256
 */

typedef struct uc_sha_256_ctx_t
{
    uint32_t W[UC_SHA256_MESSAGE_SCHEDULE_SIZE];   /* message schedule */
    uint32_t H[UC_SHA256_DIGEST_SIZE / 4];         /* hash value */

    uint8_t block[UC_SHA256_MESSAGE_BLOCK_SIZE];   /* current message block */
    uint32_t block_index;                       /* index into message block where to write next */

    uint64_t message_length;                    /* total message length in bits */

    int computed;
    int corrupted;
} uc_sha_256_ctx_t;

int uc_sha256_init(uc_sha_256_ctx_t *ctx);
int uc_sha256_reset(uc_sha_256_ctx_t *ctx);
int uc_sha256_update(uc_sha_256_ctx_t *ctx, uint8_t *message, uint64_t nbytes);
int uc_sha256_finalize(uc_sha_256_ctx_t *ctx);
int uc_sha256_finalize_with_bits(uc_sha_256_ctx_t *ctx, uint8_t data, uint64_t nbits);
int uc_sha256_output(uc_sha_256_ctx_t *ctx, uint8_t *result);

/*
 * Generic SHA
 */

typedef struct uc_sha_ctx_t
{
    uc_sha_version_t sha_version;
    union {
        uc_sha_256_ctx_t sha256_ctx;
    } ctx;
} uc_sha_ctx_t;

int uc_sha_init(uc_sha_ctx_t *ctx, uc_sha_version_t sha_version);
int uc_sha_reset(uc_sha_ctx_t *ctx);
int uc_sha_update(uc_sha_ctx_t *ctx, uint8_t *message, uint64_t nbytes);
int uc_sha_finalize(uc_sha_ctx_t *ctx);
int uc_sha_finalize_with_bits(uc_sha_ctx_t *ctx, uint8_t data, uint64_t nbits);
int uc_sha_output(uc_sha_ctx_t *ctx, uint8_t *result);

int uc_sha_message_block_length(uc_sha_ctx_t *ctx, int *length);
int uc_sha_digest_length(uc_sha_ctx_t *ctx, int *length);

/*
 * SHA-HMAC
 */

typedef struct uc_sha_hmac_ctx_t
{
    uc_sha_ctx_t sha_ctx;
    uint8_t key[UC_SHA_MAX_MESSAGE_BLOCK_SIZE];
    int sha_digest_length;
    int sha_message_block_length;
} uc_sha_hmac_ctx_t;

int uc_sha_hmac_init(uc_sha_hmac_ctx_t *ctx, uc_sha_version_t sha_version, uint8_t *key, int key_len);
int uc_sha_hmac_update(uc_sha_hmac_ctx_t *ctx, uint8_t *message, uint64_t nbytes);
int uc_sha_hmac_finalize(uc_sha_hmac_ctx_t *ctx);
int uc_sha_hmac_output(uc_sha_hmac_ctx_t *ctx, uint8_t *result);

#endif //UCRYPT_SHA_H