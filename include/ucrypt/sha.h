//
// Created by urs on 6/22/23.
//

#ifndef UCRYPT_SHA_H
#define UCRYPT_SHA_H

#include <stdint.h>
#include <ucrypt/ucdef.h>

#define SHA256_BLOCK_SIZE 512       /* SHA-256 block size in bits */
#define SHA256_DIGEST_SIZE 256      /* SHA-256 hash size in bits */
#define SHA256_MESSAGE_BLOCK_SIZE 64

typedef struct uc_sha_256_ctx_t
{
    uint32_t W[64];             /* message schedule */
    uint32_t H[8];              /* hash value */

    uint8_t block[SHA256_MESSAGE_BLOCK_SIZE];         /* current message block */
    uint32_t block_index;                             /* index into message block where to write next */

    uint64_t message_length;    /* total message length in bits */

    int computed;

} uc_sha_256_ctx_t;

int uc_sha256_init(uc_sha_256_ctx_t *ctx);
int uc_sha256_update(uc_sha_256_ctx_t *ctx, uint8_t *data, uint64_t nbytes);
int uc_sha256_update_final_bits(uc_sha_256_ctx_t *ctx, uint8_t data, uint64_t nbits);
int uc_sha256_finalize(uc_sha_256_ctx_t *ctx, uint8_t pad_byte);

#endif //UCRYPT_SHA_H
