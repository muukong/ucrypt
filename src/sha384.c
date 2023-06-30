#include <ucrypt/sha.h>

int uc_sha384_init(uc_sha_384_ctx_t *ctx)
{
    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    ctx->H[0] = 0xcbbb9d5dc1059ed8ll;
    ctx->H[1] = 0x629a292a367cd507ll;
    ctx->H[2] = 0x9159015a3070dd17ll;
    ctx->H[3] = 0x152fecd8f70e5939ll;
    ctx->H[4] = 0x67332667ffc00b31ll;
    ctx->H[5] = 0x8eb44a8768581511ll;
    ctx->H[6] = 0xdb0c2e0d64f98fa7ll;
    ctx->H[7] = 0x47b5481dbefa4fa4ll;

    ctx->block_index = 0;
    ctx->message_length_low = 0;
    ctx->message_length_high = 0;
    ctx->computed = 0;
    ctx->corrupted = 0;

    return UC_SHA_OK;
}

int uc_sha384_reset(uc_sha_384_ctx_t *ctx)
{
    int i;

    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    /* Clear message schedule and blocks (these might contain sensitive data) */
    for ( i = 0; i < UC_SHA512_MESSAGE_BLOCK_SIZE; ++i )
        ctx->block[i] = 0;

    /* Rest remaining fields */
    return uc_sha384_init(ctx);
}

int uc_sha384_update(uc_sha_384_ctx_t *ctx, uint8_t *message, uint64_t nbytes)
{
    return uc_sha512_update(ctx, message, nbytes);
}

int uc_sha384_finalize(uc_sha_384_ctx_t *ctx)
{
    return uc_sha512_finalize(ctx);
}

int uc_sha384_finalize_with_bits(uc_sha_384_ctx_t *ctx, uint8_t data, uint64_t nbits)
{
    return uc_sha512_finalize_with_bits(ctx, data, nbits);
}

int uc_sha384_output(uc_sha_384_ctx_t *ctx, uint8_t *result)
{
    int t, t8;

    if ( !ctx || !result )
        return UC_SHA_NULL_ERROR;

    if ( !ctx->computed || ctx->corrupted )
        return UC_SHA_STATE_ERROR;

    for ( t = t8 = 0; t < UC_SHA384_DIGEST_SIZE / 8; ++t, t8 += 8 )
    {
        result[t8] = (uint8_t) (ctx->H[t] >> 56);
        result[t8 + 1] = (uint8_t) (ctx->H[t] >> 48);
        result[t8 + 2] = (uint8_t) (ctx->H[t] >> 40);
        result[t8 + 3] = (uint8_t) (ctx->H[t] >> 32);
        result[t8 + 4] = (uint8_t) (ctx->H[t] >> 24);
        result[t8 + 5] = (uint8_t) (ctx->H[t] >> 16);
        result[t8 + 6] = (uint8_t) (ctx->H[t] >> 8);
        result[t8 + 7] = (uint8_t) ctx->H[t];
    }

    return UC_SHA_OK;
}