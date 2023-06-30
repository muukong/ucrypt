#include <ucrypt/sha.h>

int uc_sha224_init(uc_sha_224_ctx_t *ctx)
{
    uint32_t *h;

    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    h = ctx->H;
    h[0] = 0xc1059ed8u;
    h[1] = 0x367cd507u;
    h[2] = 0x3070dd17u;
    h[3] = 0xf70e5939u;
    h[4] = 0xffc00b31u;
    h[5] = 0x68581511u;
    h[6] = 0x64f98fa7u;
    h[7] = 0xbefa4fa4u;

    ctx->block_index = 0;
    ctx->message_length = 0;
    ctx->computed = 0;
    ctx->corrupted = 0;

    return UC_SHA_OK;
}

int uc_sha224_reset(uc_sha_224_ctx_t *ctx)
{
    int i;

    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    /* Clear message schedule and blocks (these might contain sensitive data) */

//    for ( i = 0; i < UC_SHA256_MESSAGE_SCHEDULE_SIZE; ++i )
//        ctx->W[i] = 0;
    for ( i = 0; i < UC_SHA256_MESSAGE_BLOCK_SIZE; ++i )
        ctx->block[i] = 0;

    /* Rest remaining fields */

    return uc_sha224_init(ctx);
}

int uc_sha224_update(uc_sha_224_ctx_t *ctx, uint8_t *message, uint64_t nbytes)
{
    return uc_sha256_update(ctx, message, nbytes);
}

int uc_sha224_finalize(uc_sha_224_ctx_t *ctx)
{
    return uc_sha256_finalize(ctx);
}

int uc_sha224_finalize_with_bits(uc_sha_224_ctx_t *ctx, uint8_t data, uint64_t nbits)
{
    return uc_sha256_finalize_with_bits(ctx, data, nbits);
}

int uc_sha224_output(uc_sha_224_ctx_t *ctx, uint8_t *result)
{
    int t, t4;

    if ( !ctx || !result )
        return UC_SHA_NULL_ERROR;

    if ( !ctx->computed || ctx->corrupted )
        return UC_SHA_STATE_ERROR;

    for ( t = t4 = 0; t < 7; ++t, t4 += 4 )
    {
        result[t4] = (uint8_t) (ctx->H[t] >> 24);
        result[t4 + 1] = (uint8_t) (ctx->H[t] >> 16);
        result[t4 + 2] = (uint8_t) (ctx->H[t] >> 8);
        result[t4 + 3] = (uint8_t) (ctx->H[t]);
    }

    return UC_SHA_OK;
}