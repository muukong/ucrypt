#include <ucrypt/sha.h>
#include <stdio.h>

void print_in_hex(uint8_t *data, uint64_t size)
{
    uint64_t i;

    for ( i = 0; i < size; ++i )
        printf("%02x", data[i]);
    puts("");
}

int uc_sha_hmac_init(uc_sha_hmac_ctx_t *ctx, uc_sha_version_t sha_version, uint8_t *key, int key_len)
{
    int i, res;
    uint8_t block[UC_SHA_MAX_MESSAGE_BLOCK_SIZE];

    /* Initialize underlying SHA context */
    if ((res = uc_sha_init(&ctx->sha_ctx, sha_version)) != UC_SHA_OK )
        return res;

    /* Initialize digest length field */
    if ((res = uc_sha_digest_length(&ctx->sha_ctx, &ctx->sha_digest_length)) != UC_SHA_OK )
        return res;

    /* Initialize message block length field */
    if ((res = uc_sha_message_block_length(&ctx->sha_ctx, &ctx->sha_message_block_length)) != UC_SHA_OK )
        return res;

    /* If key is larger than message block size, hash the key and use resulting digest as key */
    if ( key_len > ctx->sha_message_block_length )
    {
        /* Hash key */
        if ((res = uc_sha_update(&ctx->sha_ctx, key, key_len)) != UC_SHA_OK ||
            (res = uc_sha_finalize(&ctx->sha_ctx)) != UC_SHA_OK ||
            (res = uc_sha_output(&ctx->sha_ctx, ctx->key)) != UC_SHA_OK ||
            (res = uc_sha_reset(&ctx->sha_ctx)) != UC_SHA_OK )
        {
            return res;
        }

        /* Pad remaining bytes with 0's */
        for ( i = ctx->sha_digest_length; i < ctx->sha_message_block_length; ++i )
            ctx->key[i] = 0;
    }
    else /* copy key and pad if necessary */
    {
        for ( i = 0; i < key_len; ++i )
            ctx->key[i] = key[i];

        /* pad key with 0s (if necessary) */
        for ( ; i < ctx->sha_message_block_length; ++i )
            ctx->key[i] = 0;
    }

    /* Compute H(K XOR ipad) */
    for ( i = 0; i < ctx->sha_message_block_length; ++i )
        block[i] = ctx->key[i] ^ 0x36;

    return uc_sha_update(&ctx->sha_ctx, block, ctx->sha_message_block_length);
}

int uc_sha_hmac_update(uc_sha_hmac_ctx_t *ctx, uint8_t *message, uint64_t nbytes)
{
    return uc_sha_update(&ctx->sha_ctx, message, nbytes);
}

int uc_sha_hmac_finalize(uc_sha_hmac_ctx_t *ctx)
{
    int i, res;
    uint8_t inner_hash[UC_SHA_MAX_MESSAGE_BLOCK_SIZE]; /* Holds H( (K XOR ipad) || m ) */
    uint8_t block[UC_SHA_MAX_MESSAGE_BLOCK_SIZE];

    uc_sha_finalize(&ctx->sha_ctx);
    uc_sha_output(&ctx->sha_ctx, inner_hash);

    /* Now inner_hash holds H( (K XOR ipad) || m ) */

    if ( (res = uc_sha_reset(&ctx->sha_ctx)) != UC_OK )
        return res;

    /* Compute H(K XOR opad) */
    for ( i = 0; i < ctx->sha_message_block_length; ++i )
        block[i] = ctx->key[i] ^ 0x5c;

    if ( (res = uc_sha_update(&ctx->sha_ctx, block, ctx->sha_message_block_length)) != UC_OK ||
         (res = uc_sha_update(&ctx->sha_ctx, inner_hash, ctx->sha_digest_length)) != UC_OK ||
         (res = uc_sha_finalize(&ctx->sha_ctx)) != UC_OK )
    {
        return res;
    }

    return UC_SHA_OK;
}

int uc_sha_hmac_output(uc_sha_hmac_ctx_t *ctx, uint8_t *result)
{
    return uc_sha_output(&ctx->sha_ctx, result);
}
