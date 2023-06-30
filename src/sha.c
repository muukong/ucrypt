#include <ucrypt/sha.h>

int uc_sha_init(uc_sha_ctx_t *ctx, uc_sha_version_t sha_version)
{
    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    ctx->sha_version = sha_version;
    switch ( sha_version )
    {
        case UC_SHA224:     return uc_sha224_reset((uc_sha_224_ctx_t  *) &ctx->ctx);
        case UC_SHA256:     return uc_sha256_reset((uc_sha_256_ctx_t  *) &ctx->ctx);
        case UC_SHA384:     return uc_sha384_reset((uc_sha_384_ctx_t  *) &ctx->ctx);
        case UC_SHA512:     return uc_sha512_reset((uc_sha_512_ctx_t  *) &ctx->ctx);
        default:            return UC_SHA_INPUT_ERROR;
    }
}

int uc_sha_reset(uc_sha_ctx_t *ctx)
{
    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    switch ( ctx->sha_version )
    {
        case UC_SHA224:     return uc_sha224_reset((uc_sha_224_ctx_t *) &ctx->ctx);
        case UC_SHA256:     return uc_sha256_reset((uc_sha_256_ctx_t *) &ctx->ctx);
        case UC_SHA384:     return uc_sha384_reset((uc_sha_384_ctx_t *) &ctx->ctx);
        case UC_SHA512:     return uc_sha512_reset((uc_sha_512_ctx_t *) &ctx->ctx);
        default:            return UC_SHA_INPUT_ERROR;
    }
}

int uc_sha_update(uc_sha_ctx_t *ctx, uint8_t *message, uint64_t nbytes)
{
    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    switch ( ctx->sha_version )
    {
        case UC_SHA224:     return uc_sha224_update((uc_sha_224_ctx_t  *) &ctx->ctx, message, nbytes);
        case UC_SHA256:     return uc_sha256_update((uc_sha_256_ctx_t  *) &ctx->ctx, message, nbytes);
        case UC_SHA384:     return uc_sha384_update((uc_sha_384_ctx_t  *) &ctx->ctx, message, nbytes);
        case UC_SHA512:     return uc_sha512_update((uc_sha_512_ctx_t  *) &ctx->ctx, message, nbytes);
        default:            return UC_SHA_INPUT_ERROR;
    }
}

int uc_sha_finalize(uc_sha_ctx_t *ctx)
{
    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    switch ( ctx->sha_version )
    {
        case UC_SHA224:     return uc_sha224_finalize((uc_sha_224_ctx_t *) &ctx->ctx);
        case UC_SHA256:     return uc_sha256_finalize((uc_sha_256_ctx_t *) &ctx->ctx);
        case UC_SHA384:     return uc_sha384_finalize((uc_sha_384_ctx_t *) &ctx->ctx);
        case UC_SHA512:     return uc_sha512_finalize((uc_sha_512_ctx_t *) &ctx->ctx);
        default:            return UC_SHA_INPUT_ERROR;
    }
}

int uc_sha_finalize_with_bits(uc_sha_ctx_t *ctx, uint8_t data, uint64_t nbits)
{
    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    switch ( ctx->sha_version )
    {
        case UC_SHA224:     return uc_sha224_finalize_with_bits((uc_sha_224_ctx_t *) &ctx->ctx, data, nbits);
        case UC_SHA256:     return uc_sha256_finalize_with_bits((uc_sha_256_ctx_t *) &ctx->ctx, data, nbits);
        case UC_SHA384:     return uc_sha384_finalize_with_bits((uc_sha_384_ctx_t *) &ctx->ctx, data, nbits);
        case UC_SHA512:     return uc_sha512_finalize_with_bits((uc_sha_512_ctx_t *) &ctx->ctx, data, nbits);
        default:            return UC_SHA_INPUT_ERROR;
    }
}

int uc_sha_output(uc_sha_ctx_t *ctx, uint8_t *result)
{
    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    switch ( ctx->sha_version )
    {
        case UC_SHA224:     return uc_sha224_output( (uc_sha_224_ctx_t *) &ctx->ctx, result);
        case UC_SHA256:     return uc_sha256_output( (uc_sha_256_ctx_t *) &ctx->ctx, result);
        case UC_SHA384:     return uc_sha384_output( (uc_sha_384_ctx_t *) &ctx->ctx, result);
        case UC_SHA512:     return uc_sha512_output( (uc_sha_512_ctx_t *) &ctx->ctx, result);
        default:            return UC_SHA_INPUT_ERROR;
    }
}

int uc_sha_message_block_length(uc_sha_ctx_t *ctx, int *length)
{
    if ( !ctx || !length )
        return UC_SHA_NULL_ERROR;

    switch ( ctx->sha_version )
    {
        case UC_SHA224:
            *length = UC_SHA224_MESSAGE_BLOCK_SIZE;
            return UC_SHA_OK;
        case UC_SHA256:
            *length = UC_SHA256_MESSAGE_BLOCK_SIZE;
            return UC_SHA_OK;
        case UC_SHA384:
            *length = UC_SHA384_MESSAGE_BLOCK_SIZE;
            return UC_SHA_OK;
         case UC_SHA512:
            *length = UC_SHA512_MESSAGE_BLOCK_SIZE;
            return UC_SHA_OK;
        default:
            return UC_SHA_INPUT_ERROR;
    }
}

int uc_sha_digest_length(uc_sha_ctx_t *ctx, int *length)
{
    if ( !ctx || !length )
        return UC_SHA_NULL_ERROR;

    switch ( ctx->sha_version )
    {
        case UC_SHA224:
            *length = UC_SHA224_DIGEST_SIZE;
            return UC_SHA_OK;
        case UC_SHA256:
            *length = UC_SHA256_DIGEST_SIZE;
            return UC_SHA_OK;
        case UC_SHA384:
            *length = UC_SHA384_DIGEST_SIZE;
            return UC_SHA_OK;
         case UC_SHA512:
            *length = UC_SHA512_DIGEST_SIZE;
            return UC_SHA_OK;
        default:
            return UC_SHA_INPUT_ERROR;
    }
}