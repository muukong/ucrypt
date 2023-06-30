#include <stdint.h>
#include <ucrypt/sha.h>

#define SHR(n, x) ((x) >> (n))
#define ROTL(n,x) ( ((x) << (n)) | ((x) >> (32-(n))) )
#define ROTR(n,x) ( ((x) >> (n)) | ((x) << (32-(n))) )

#define CH(x,y,z) ( ((x) & (y)) ^ ( (~(x)) & (z)) )
#define PARITY(x,y,z) ( (x) ^ (y) ^ (z) )
#define MAJ(x,y,z) ( ((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)) )


static void _uc_sha1_transform_block(uc_sha_1_ctx_t *ctx);
static void _uc_sha1_finalize(uc_sha_1_ctx_t *ctx, uint8_t pad_byte);
static void _uc_sha1_pad_message(uc_sha_1_ctx_t *ctx, uint8_t pad_byte);

static const uint32_t K[4] = {
        0x5a827999,     /* 0 <= t < 20 */
        0x6ed9eba1,     /* 20 <= t < 40 */
        0x8f1bbcdc,     /* 40 <= t < 60 */
        0xca62c1d6      /* 40 <= t < 80 */
};

int uc_sha1_init(uc_sha_1_ctx_t *ctx)
{
    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    ctx->H[0] = 0x67452301;
    ctx->H[1] = 0xefcdab89;
    ctx->H[2] = 0x98badcfe;
    ctx->H[3] = 0x10325476;
    ctx->H[4] = 0xc3d2e1f0;

    ctx->block_index = 0;
    ctx->message_length = 0;
    ctx->computed = 0;
    ctx->corrupted = 0;

    return UC_SHA_OK;
}
int uc_sha1_reset(uc_sha_1_ctx_t *ctx)
{
   int i;

    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    /* Clear message schedule and blocks (these might contain sensitive data) */
    for ( i = 0; i < UC_SHA1_MESSAGE_BLOCK_SIZE; ++i )
        ctx->block[i] = 0;

    /* Rest remaining fields */
    return uc_sha1_init(ctx);
}

int uc_sha1_update(uc_sha_1_ctx_t *ctx, uint8_t *message, uint64_t nbytes)
{
    uint64_t i;

    if ( !nbytes )
        return UC_OK;

    if ( !ctx || !message )
        return UC_SHA_NULL_ERROR;

    if ( ctx->computed || ctx->corrupted )
        return UC_SHA_STATE_ERROR;

    ctx->message_length += 8 * nbytes;

    for ( i = 0; i < nbytes; ++i )
    {
        ctx->block[ctx->block_index++] = *message;

        if (ctx->block_index == UC_SHA1_MESSAGE_BLOCK_SIZE )
            _uc_sha1_transform_block(ctx);

        ++message;
    }

    return UC_SHA_OK;
}

void _uc_sha1_transform_block(uc_sha_1_ctx_t *ctx)
{
    int t, t4;
    uint32_t W[80];   /* message schedule */
    uint32_t a, b, c, d, e, T, F;                      /* working variables */
    uint32_t *H;

    H = ctx->H;

    /* Prepare message schedule w */

    for ( t = t4 = 0; t < 16; ++t, t4 += 4 )
    {
        W[t] = ((uint32_t) ctx->block[t4]) << 24 |
               ((uint32_t) ctx->block[t4 + 1]) << 16 |
               ((uint32_t) ctx->block[t4 + 2]) << 8 |
               ((uint32_t) ctx->block[t4 + 3]);
    }

    for ( ; t < 80; ++t)
        W[t] = ROTL(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);

    /* Initialize working variables */

    a = H[0];
    b = H[1];
    c = H[2];
    d = H[3];
    e = H[4];

    /* Perform main hash computation */

    for ( t = 0; t < 80; ++t )
    {
        /*
         * Evaluate function f (which depends on t) according to
         * https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf, Section 4.1.1
         */
        if ( 0 <= t && t <= 19 )
            F = CH(b, c, d);
        else if ( 20 <= t && t <= 39 )
            F = PARITY(b, c, d);
        else if ( 40 <= t && t <= 59 )
            F = MAJ(b, c, d);
        else
            F = PARITY(b, c, d);

        T = ROTL(5,a) + F + e + K[t / 20] + W[t];
        e = d;
        d = c;
        c = ROTL(30,b);
        b = a;
        a = T;
    }

    /* Compute intermediate hash values */

    H[0] += a;
    H[1] += b;
    H[2] += c;
    H[3] += d;
    H[4] += e;

    /* Reset block index */
    ctx->block_index = 0;
}

int uc_sha1_finalize(uc_sha_1_ctx_t *ctx)
{
    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    if ( ctx->computed || ctx->corrupted )
        return UC_SHA_STATE_ERROR;

    _uc_sha1_finalize(ctx, 0x80);
    return UC_SHA_OK;
}

int uc_sha1_finalize_with_bits(uc_sha_1_ctx_t *ctx, uint8_t data, uint64_t nbits)
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
    _uc_sha1_finalize(ctx, (uint8_t) ((data & masks[nbits]) | mark_bits[nbits]));
    return UC_SHA_OK;
}

int uc_sha1_output(uc_sha_1_ctx_t *ctx, uint8_t *result)
{
    int t, t4;

    if ( !ctx || !result )
        return UC_SHA_NULL_ERROR;

    if ( !ctx->computed || ctx->corrupted )
        return UC_SHA_STATE_ERROR;

    for ( t = t4 = 0; t < UC_SHA1_DIGEST_SIZE / 4; ++t, t4 += 4 )
    {
        result[t4] = (uint8_t) (ctx->H[t] >> 24);
        result[t4 + 1] = (uint8_t) (ctx->H[t] >> 16);
        result[t4 + 2] = (uint8_t) (ctx->H[t] >> 8);
        result[t4 + 3] = (uint8_t) (ctx->H[t]);
    }

    return UC_SHA_OK;
}

static void _uc_sha1_finalize(uc_sha_1_ctx_t *ctx, uint8_t pad_byte)
{
    int i;

    _uc_sha1_pad_message(ctx, pad_byte);

    /* Clear message bits which might contain sensitive data */
    for ( i = 0; i < UC_SHA1_MESSAGE_BLOCK_SIZE; ++i )
        ctx->block[i] = 0;

    ctx->message_length = 0;
    ctx->computed = 1;
}

void _uc_sha1_pad_message(uc_sha_1_ctx_t *ctx, uint8_t pad_byte)
{
    if (ctx->block_index >= UC_SHA1_MESSAGE_BLOCK_SIZE - 8)
    {
        ctx->block[ctx->block_index++] = pad_byte;
        while (ctx->block_index < UC_SHA1_MESSAGE_BLOCK_SIZE )
            ctx->block[ctx->block_index++] = 0;

        _uc_sha1_transform_block(ctx);
    }
    else
        ctx->block[ctx->block_index++] = pad_byte;

    while (ctx->block_index < UC_SHA1_MESSAGE_BLOCK_SIZE - 8 )
        ctx->block[ctx->block_index++] = 0;

    ctx->block[56] = (uint8_t) (ctx->message_length >> 56);
    ctx->block[57] = (uint8_t) (ctx->message_length >> 48);
    ctx->block[58] = (uint8_t) (ctx->message_length >> 40);
    ctx->block[59] = (uint8_t) (ctx->message_length >> 32);
    ctx->block[60] = (uint8_t) (ctx->message_length >> 24);
    ctx->block[61] = (uint8_t) (ctx->message_length >> 16);
    ctx->block[62] = (uint8_t) (ctx->message_length >> 8);
    ctx->block[63] = (uint8_t) (ctx->message_length);

    _uc_sha1_transform_block(ctx);
}
