#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <ucrypt/sha.h>

#define SHR(n, x) ((x) >> (n))
#define ROTR(n,x) ( ((x) >> (n)) | ((x) << (32-(n))) )

#define CH(x,y,z) ( ((x) & (y)) ^ ( (~(x)) & (z)) )
#define MAJ(x,y,z) ( ((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)) )
#define BSIG0(x) ( ROTR(2,(x)) ^ ROTR(13,(x)) ^ ROTR(22,(x)) )
#define BSIG1(x) ( ROTR(6,(x)) ^ ROTR(11,(x)) ^ ROTR(25,(x)) )
#define SSIG0(x) ( ROTR(7,(x)) ^ ROTR(18,(x)) ^ SHR(3,(x)) )
#define SSIG1(x) ( ROTR(17,(x)) ^ ROTR(19,(x)) ^ SHR(10,(x)) )

static void _uc_sha256_transform_block(uc_sha_256_ctx_t *ctx);
static void _uc_sha256_finalize(uc_sha_256_ctx_t *ctx, uint8_t pad_byte);
static void _uc_sha256_pad_message(uc_sha_256_ctx_t *ctx, uint8_t pad_byte);

const uint32_t K[] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

int uc_sha256_init(uc_sha_256_ctx_t *ctx)
{
    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    uint32_t *h = ctx->H;
    h[0] = 0x6a09e667;
    h[1] = 0xbb67ae85;
    h[2] = 0x3c6ef372;
    h[3] = 0xa54ff53a;
    h[4] = 0x510e527f;
    h[5] = 0x9b05688c;
    h[6] = 0x1f83d9ab;
    h[7] = 0x5be0cd19;

    ctx->block_index = 0;
    ctx->message_length = 0;
    ctx->computed = 0;
    ctx->corrupted = 0;

    return UC_SHA_OK;
}

int uc_sha256_reset(uc_sha_256_ctx_t *ctx)
{
    int i;

    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    /* Clear message schedule and blocks (these might contain sensitive data) */

    for ( i = 0; i < UC_SHA256_MESSAGE_SCHEDULE_SIZE; ++i )
        ctx->W[i] = 0;
    for ( i = 0; i < UC_SHA256_MESSAGE_BLOCK_SIZE; ++i )
        ctx->block[i] = 0;

    /* Rest remaining fields */

    return uc_sha256_init(ctx);
}


int uc_sha256_update(uc_sha_256_ctx_t *ctx, uint8_t *message, uint64_t nbytes)
{
    if ( !nbytes )
        return UC_OK;

    if ( !ctx || !message )
        return UC_SHA_NULL_ERROR;

    if ( ctx->computed || ctx->corrupted )
        return UC_SHA_STATE_ERROR;

    ctx->message_length += 8 * nbytes;

    while ( nbytes-- > 0 )
    {
        ctx->block[ctx->block_index++] = *message;

        if (ctx->block_index == UC_SHA256_MESSAGE_BLOCK_SIZE )
            _uc_sha256_transform_block(ctx);

        ++message;
    }

    return UC_SHA_OK;
}

int uc_sha256_finalize(uc_sha_256_ctx_t *ctx)
{
    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    if ( ctx->computed || ctx->corrupted )
        return UC_SHA_STATE_ERROR;

    _uc_sha256_finalize(ctx, 0x80);
    return UC_SHA_OK;
}

int uc_sha256_finalize_with_bits(uc_sha_256_ctx_t *ctx, uint8_t data, uint64_t nbits)
{
    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    if ( nbits >= 8 )
        return UC_SHA_INPUT_ERROR;

    const uint8_t masks[8] = {
            0x00,   /* 0 0b00000000 */
            0x80,   /* 1 0b10000000 */
            0xc0,   /* 2 0b11000000 */
            0xe0,   /* 3 0b11100000 */
            0xf0,   /* 4 0b11110000 */
            0xf8,   /* 5 0b11111000 */
            0xfc,   /* 6 0b11111100 */
            0xfe    /* 7 0b11111110 */
    };

    const uint8_t mark_bits[8] = {
            0x80,   /* 0 0b10000000 */
            0x40,   /* 1 0b01000000 */
            0x20,   /* 2 0b00100000 */
            0x10,   /* 3 0b00010000 */
            0x08,   /* 4 0b00001000 */
            0x04,   /* 5 0b00000100 */
            0x02,   /* 6 0b00000010 */
            0x01    /* 7 0b00000001 */
    };

    ctx->message_length += nbits;
    _uc_sha256_finalize(ctx, (uint8_t) ((data & masks[nbits]) | mark_bits[nbits]));
    return UC_SHA_OK;
}

int uc_sha256_output(uc_sha_256_ctx_t *ctx, uint8_t *result)
{
    int t, t4;

    if ( !ctx || !result )
        return UC_SHA_NULL_ERROR;

    if ( !ctx->computed || ctx->corrupted )
        return UC_SHA_STATE_ERROR;

    for ( t = t4 = 0; t < 8; ++t, t4 += 4 )
    {
        result[t4] = (uint8_t) (ctx->H[t] >> 24);
        result[t4 + 1] = (uint8_t) (ctx->H[t] >> 16);
        result[t4 + 2] = (uint8_t) (ctx->H[t] >> 8);
        result[t4 + 3] = (uint8_t) (ctx->H[t]);
    }

    return UC_SHA_STATE_ERROR;
}

void _uc_sha256_transform_block(uc_sha_256_ctx_t *ctx)
{
    int t, t4;
    uint32_t a, b, c, d, e, f, g, h, T1, T2;
    uint32_t *H, *W;

    H = ctx->H;
    W = ctx->W;

    /* Prepare message schedule w */

    for ( t = t4 = 0; t < 16; ++t, t4 += 4 )
    {
        W[t] = ((uint32_t) ctx->block[t4]) << 24 |
               ((uint32_t) ctx->block[t4 + 1]) << 16 |
               ((uint32_t) ctx->block[t4 + 2]) << 8 |
               ((uint32_t) ctx->block[t4 + 3]);
    }

    for ( t = 16; t < 64; ++t )
        W[t] = SSIG1(W[t-2]) + W[t-7] + SSIG0(W[t-15]) + W[t-16];

    /* Initialize working variables */

    a = H[0];
    b = H[1];
    c = H[2];
    d = H[3];
    e = H[4];
    f = H[5];
    g = H[6];
    h = H[7];

    /* Perform main hash computation */

    for ( t = 0; t < 64; ++t )
    {
        T1 = h + BSIG1(e) + CH(e,f,g) + K[t] + W[t];
        T2 = BSIG0(a) + MAJ(a,b,c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    /* Compute intermediate hash values */
    H[0] += a;
    H[1] += b;
    H[2] += c;
    H[3] += d;
    H[4] += e;
    H[5] += f;
    H[6] += g;
    H[7] += h;

    ctx->block_index = 0;
}

void _uc_sha256_finalize(uc_sha_256_ctx_t *ctx, uint8_t pad_byte)
{
    int i;

    _uc_sha256_pad_message(ctx, pad_byte);

    /* Clear message bits which might contain sensitive data */
    for ( i = 0; i < UC_SHA256_MESSAGE_BLOCK_SIZE; ++i )
        ctx->block[i] = 0;

    ctx->message_length = 0;
    ctx->computed = 1;
}

void _uc_sha256_pad_message(uc_sha_256_ctx_t *ctx, uint8_t pad_byte)
{

    if (ctx->block_index >= UC_SHA256_MESSAGE_BLOCK_SIZE - 8)
    {
        ctx->block[ctx->block_index++] = pad_byte;
        while (ctx->block_index < UC_SHA256_MESSAGE_BLOCK_SIZE )
            ctx->block[ctx->block_index++] = 0;

        _uc_sha256_transform_block(ctx);
    }
    else
        ctx->block[ctx->block_index++] = pad_byte;

    while (ctx->block_index < UC_SHA256_MESSAGE_BLOCK_SIZE - 8 )
        ctx->block[ctx->block_index++] = 0;

    ctx->block[56] = (uint8_t) (ctx->message_length >> 56);
    ctx->block[57] = (uint8_t) (ctx->message_length >> 48);
    ctx->block[58] = (uint8_t) (ctx->message_length >> 40);
    ctx->block[59] = (uint8_t) (ctx->message_length >> 32);
    ctx->block[60] = (uint8_t) (ctx->message_length >> 24);
    ctx->block[61] = (uint8_t) (ctx->message_length >> 16);
    ctx->block[62] = (uint8_t) (ctx->message_length >> 8);
    ctx->block[63] = (uint8_t) (ctx->message_length);

    _uc_sha256_transform_block(ctx);
}
