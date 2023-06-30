#include <ucrypt/sha.h>
#include <stdio.h>

#define SHR(n, x) (((uint64_t) x) >> (n))
#define ROTR(n,x) ( ((uint64_t) (x) >> (n)) | ((uint64_t) (x) << (64-(n))) )

#define CH(x,y,z) ( ((x) & (y)) ^ ( (~(x)) & (z)) )
#define MAJ(x,y,z) ( ((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)) )
#define BSIG0(x) ( ROTR(28,(x)) ^ ROTR(34,(x)) ^ ROTR(39,(x)) )
#define BSIG1(x) ( ROTR(14,(x)) ^ ROTR(18,(x)) ^ ROTR(41,(x)) )
#define SSIG0(x) ( ROTR(1,(x)) ^ ROTR(8,(x)) ^ SHR(7,(x)) )
#define SSIG1(x) ( ROTR(19,(x)) ^ ROTR(61,(x)) ^ SHR(6,(x)) )

static const uint64_t K[80] = {
        0x428A2F98D728AE22ll, 0x7137449123EF65CDll, 0xB5C0FBCFEC4D3B2Fll,
        0xE9B5DBA58189DBBCll, 0x3956C25BF348B538ll, 0x59F111F1B605D019ll,
        0x923F82A4AF194F9Bll, 0xAB1C5ED5DA6D8118ll, 0xD807AA98A3030242ll,
        0x12835B0145706FBEll, 0x243185BE4EE4B28Cll, 0x550C7DC3D5FFB4E2ll,
        0x72BE5D74F27B896Fll, 0x80DEB1FE3B1696B1ll, 0x9BDC06A725C71235ll,
        0xC19BF174CF692694ll, 0xE49B69C19EF14AD2ll, 0xEFBE4786384F25E3ll,
        0x0FC19DC68B8CD5B5ll, 0x240CA1CC77AC9C65ll, 0x2DE92C6F592B0275ll,
        0x4A7484AA6EA6E483ll, 0x5CB0A9DCBD41FBD4ll, 0x76F988DA831153B5ll,
        0x983E5152EE66DFABll, 0xA831C66D2DB43210ll, 0xB00327C898FB213Fll,
        0xBF597FC7BEEF0EE4ll, 0xC6E00BF33DA88FC2ll, 0xD5A79147930AA725ll,
        0x06CA6351E003826Fll, 0x142929670A0E6E70ll, 0x27B70A8546D22FFCll,
        0x2E1B21385C26C926ll, 0x4D2C6DFC5AC42AEDll, 0x53380D139D95B3DFll,
        0x650A73548BAF63DEll, 0x766A0ABB3C77B2A8ll, 0x81C2C92E47EDAEE6ll,
        0x92722C851482353Bll, 0xA2BFE8A14CF10364ll, 0xA81A664BBC423001ll,
        0xC24B8B70D0F89791ll, 0xC76C51A30654BE30ll, 0xD192E819D6EF5218ll,
        0xD69906245565A910ll, 0xF40E35855771202All, 0x106AA07032BBD1B8ll,
        0x19A4C116B8D2D0C8ll, 0x1E376C085141AB53ll, 0x2748774CDF8EEB99ll,
        0x34B0BCB5E19B48A8ll, 0x391C0CB3C5C95A63ll, 0x4ED8AA4AE3418ACBll,
        0x5B9CCA4F7763E373ll, 0x682E6FF3D6B2B8A3ll, 0x748F82EE5DEFB2FCll,
        0x78A5636F43172F60ll, 0x84C87814A1F0AB72ll, 0x8CC702081A6439ECll,
        0x90BEFFFA23631E28ll, 0xA4506CEBDE82BDE9ll, 0xBEF9A3F7B2C67915ll,
        0xC67178F2E372532Bll, 0xCA273ECEEA26619Cll, 0xD186B8C721C0C207ll,
        0xEADA7DD6CDE0EB1Ell, 0xF57D4F7FEE6ED178ll, 0x06F067AA72176FBAll,
        0x0A637DC5A2C898A6ll, 0x113F9804BEF90DAEll, 0x1B710B35131C471Bll,
        0x28DB77F523047D84ll, 0x32CAAB7B40C72493ll, 0x3C9EBE0A15C9BEBCll,
        0x431D67C49C100D4Cll, 0x4CC5D4BECB3E42B6ll, 0x597F299CFC657E2All,
        0x5FCB6FAB3AD6FAECll, 0x6C44198C4A475817ll
};

static void _uc_sha512_transform_block(uc_sha_512_ctx_t *ctx);
static void _uc_sha512_finalize(uc_sha_512_ctx_t *ctx, uint8_t pad_byte);
static void _uc_sha512_pad_message(uc_sha_512_ctx_t *ctx, uint8_t pad_byte);
static void _uc_sha512_update_length(uc_sha_512_ctx_t *ctx, uint32_t nbytes);
void _uc_sha512_update_length_bits(uc_sha_512_ctx_t *ctx, uint32_t len_bits);

int uc_sha512_init(uc_sha_512_ctx_t *ctx)
{
    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    ctx->H[0] = 0x6a09e667f3bcc908ll;
    ctx->H[1] = 0xbb67ae8584caa73bll;
    ctx->H[2] = 0x3c6ef372fe94f82bll;
    ctx->H[3] = 0xa54ff53a5f1d36f1ll;
    ctx->H[4] = 0x510e527fade682d1ll;
    ctx->H[5] = 0x9b05688c2b3e6c1fll;
    ctx->H[6] = 0x1f83d9abfb41bd6bll;
    ctx->H[7] = 0x5be0cd19137e2179ll;

    ctx->block_index = 0;
    ctx->message_length_low = 0;
    ctx->message_length_high = 0;
    ctx->computed = 0;
    ctx->corrupted = 0;

    return UC_SHA_OK;
}

int uc_sha512_reset(uc_sha_512_ctx_t *ctx)
{
    int i;

    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    /* Clear message schedule and blocks (these might contain sensitive data) */
    for ( i = 0; i < UC_SHA512_MESSAGE_BLOCK_SIZE; ++i )
        ctx->block[i] = 0;

    /* Rest remaining fields */
    return uc_sha512_init(ctx);
}

int uc_sha512_update(uc_sha_512_ctx_t *ctx, uint8_t *message, uint32_t nbytes)
{
    uint64_t i;

    if ( !nbytes )
        return UC_SHA_OK;

    if ( !ctx || !message )
        return UC_SHA_NULL_ERROR;

    if ( ctx->computed || ctx->corrupted )
        return UC_SHA_STATE_ERROR;

    for ( i = 0; i < nbytes; ++i )
    {
        ctx->block[ctx->block_index++] = message[i];

        if ( ctx->block_index == UC_SHA512_MESSAGE_BLOCK_SIZE )
            _uc_sha512_transform_block(ctx);
    }

    _uc_sha512_update_length(ctx, nbytes);

    return UC_SHA_OK;
}

int uc_sha512_finalize(uc_sha_512_ctx_t *ctx)
{
    if ( !ctx )
        return UC_SHA_NULL_ERROR;

    if ( ctx->computed || ctx->corrupted )
        return UC_SHA_STATE_ERROR;

    _uc_sha512_finalize(ctx, 0x80);
    return UC_SHA_OK;
}

int uc_sha512_finalize_with_bits(uc_sha_512_ctx_t *ctx, uint8_t data, uint64_t nbits)
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


    _uc_sha512_update_length_bits(ctx, nbits);
    _uc_sha512_finalize(ctx, (uint8_t) ((data & masks[nbits]) | mark_bits[nbits]));
    return UC_SHA_OK;
}

int uc_sha512_output(uc_sha_512_ctx_t *ctx, uint8_t *result)
{
    int t, t8;

    if ( !ctx || !result )
        return UC_SHA_NULL_ERROR;

    if ( !ctx->computed || ctx->corrupted )
        return UC_SHA_STATE_ERROR;

    for ( t = t8 = 0; t < UC_SHA512_DIGEST_SIZE / 8; ++t, t8 += 8 )
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

void _uc_sha512_transform_block(uc_sha_512_ctx_t *ctx)
{
    int t, t8;
    uint64_t W[80];   /* message schedule */
    uint64_t a, b, c, d, e, f, g, h, T1, T2;       /* working variables */

    /* Prepare message schedule W */

    for ( t = t8 = 0; t < 16; ++t, t8 += 8 )
    {
        W[t] = ((uint64_t) ctx->block[t8]) << 56 |
               ((uint64_t) ctx->block[t8 + 1]) << 48|
               ((uint64_t) ctx->block[t8 + 2]) << 40 |
               ((uint64_t) ctx->block[t8 + 3]) << 32 |
               ((uint64_t) ctx->block[t8 + 4]) << 24 |
               ((uint64_t) ctx->block[t8 + 5]) << 16 |
               ((uint64_t) ctx->block[t8 + 6]) << 8 |
               ((uint64_t) ctx->block[t8 + 7]);
    }

    for ( t = 16; t < 80; ++t )
        W[t] = SSIG1(W[t-2]) + W[t-7] + SSIG0(W[t-15]) + W[t-16];

    /* Prepare working variables */

    a = ctx->H[0];
    b = ctx->H[1];
    c = ctx->H[2];
    d = ctx->H[3];
    e = ctx->H[4];
    f = ctx->H[5];
    g = ctx->H[6];
    h = ctx->H[7];

    /* Perform main hash computation */

    for ( t = 0; t < 80; ++t )
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

    ctx->H[0] += a;
    ctx->H[1] += b;
    ctx->H[2] += c;
    ctx->H[3] += d;
    ctx->H[4] += e;
    ctx->H[5] += f;
    ctx->H[6] += g;
    ctx->H[7] += h;

    /* Reset block index */
    ctx->block_index = 0;
}

static void _uc_sha512_finalize(uc_sha_512_ctx_t *ctx, uint8_t pad_byte)
{
    int i;

    _uc_sha512_pad_message(ctx, pad_byte);

    /* Clear message bits which might contain sensitive data */
    for ( i = 0; i < UC_SHA512_MESSAGE_BLOCK_SIZE; ++i )
        ctx->block[i] = 0;

    ctx->message_length_low = 0;
    ctx->message_length_high = 0;
    ctx->computed = 1;
}

static void _uc_sha512_pad_message(uc_sha_512_ctx_t *ctx, uint8_t pad_byte)
{
    if ( ctx->block_index >= (UC_SHA512_MESSAGE_BLOCK_SIZE - 16) )
    {
        ctx->block[ctx->block_index++] = pad_byte;
        while (ctx->block_index < UC_SHA512_MESSAGE_BLOCK_SIZE )
            ctx->block[ctx->block_index++] = 0;

        _uc_sha512_transform_block(ctx);
    }
    else
        ctx->block[ctx->block_index++] = pad_byte;

    while ( ctx->block_index < (UC_SHA512_MESSAGE_BLOCK_SIZE - 16) )
        ctx->block[ctx->block_index++] = 0;

    ctx->block[112] = (uint8_t) (ctx->message_length_high >> 56);
    ctx->block[113] = (uint8_t) (ctx->message_length_high >> 48);
    ctx->block[114] = (uint8_t) (ctx->message_length_high >> 40);
    ctx->block[115] = (uint8_t) (ctx->message_length_high >> 32);
    ctx->block[116] = (uint8_t) (ctx->message_length_high >> 24);
    ctx->block[117] = (uint8_t) (ctx->message_length_high >> 16);
    ctx->block[118] = (uint8_t) (ctx->message_length_high >> 8);
    ctx->block[119] = (uint8_t) (ctx->message_length_high);

    ctx->block[120] = (uint8_t) (ctx->message_length_low >> 56);
    ctx->block[121] = (uint8_t) (ctx->message_length_low >> 48);
    ctx->block[122] = (uint8_t) (ctx->message_length_low >> 40);
    ctx->block[123] = (uint8_t) (ctx->message_length_low >> 32);
    ctx->block[124] = (uint8_t) (ctx->message_length_low >> 24);
    ctx->block[125] = (uint8_t) (ctx->message_length_low >> 16);
    ctx->block[126] = (uint8_t) (ctx->message_length_low >> 8);
    ctx->block[127] = (uint8_t) (ctx->message_length_low);

    _uc_sha512_transform_block(ctx);
}

void _uc_sha512_update_length(uc_sha_512_ctx_t *ctx, uint32_t len)
{
    uint64_t tmp, bit_length;
    uint32_t bit_length_low, bit_length_high;

    uint32_t length_arr4[4] = {0, 0, 0, 0};

    /* Store bit length in 64 bit variable to avoid overflows, then break up into low and high part */
    bit_length = 8 * ((uint64_t) len);
    bit_length_low = (uint32_t) bit_length;
    bit_length_high = (uint32_t) (bit_length >> 32);

    /* Store current bit length in array of four 32-bit integers */
    length_arr4[3] = (uint32_t) ctx->message_length_low;
    length_arr4[2] = (uint32_t) (ctx->message_length_low >> 32);
    length_arr4[1] = (uint32_t) ctx->message_length_high;
    length_arr4[0] = (uint32_t) (ctx->message_length_high >> 32);

    /* Add bit_length_low (with carry propagation) */
    tmp = (uint64_t) length_arr4[3] + (uint64_t) bit_length_low;
    length_arr4[3] = (uint32_t) tmp;
    tmp = (uint64_t) length_arr4[2] + (tmp >> 32);
    length_arr4[2] = (uint32_t) tmp;
    tmp = (uint64_t) length_arr4[1] + (tmp >> 32);
    length_arr4[1] = (uint32_t) tmp;
    tmp = (uint64_t) length_arr4[0] + (tmp >> 32);
    length_arr4[0] = (uint32_t) tmp;

    /* Add bit_length_high (with carry propagation) */
    tmp = (uint64_t) length_arr4[2] + (uint64_t) bit_length_high;
    length_arr4[2] = (uint32_t) tmp;
    tmp = (uint64_t) length_arr4[1] + (tmp >> 32);
    length_arr4[1] = (uint32_t) tmp;
    tmp = (uint64_t) length_arr4[0] + (tmp >> 32);
    length_arr4[0] = (uint32_t) tmp;

    /* Write result to context */
    ctx->message_length_low = ((uint64_t) length_arr4[3]) + (((uint64_t) length_arr4[2]) << 32);
    ctx->message_length_high = ((uint64_t) length_arr4[1]) + (((uint64_t) length_arr4[0]) << 32);
}

void _uc_sha512_update_length_bits(uc_sha_512_ctx_t *ctx, uint32_t len_bits)
{
    uint64_t tmp, bit_length;
    uint32_t bit_length_low, bit_length_high;

    uint32_t length_arr4[4] = {0, 0, 0, 0};

    /* Store bit length in 64 bit variable to avoid overflows, then break up into low and high part */
    bit_length = ((uint64_t) len_bits);
    bit_length_low = (uint32_t) bit_length;
    bit_length_high = (uint32_t) (bit_length >> 32);

    /* Store current bit length in array of four 32-bit integers */
    length_arr4[3] = (uint32_t) ctx->message_length_low;
    length_arr4[2] = (uint32_t) (ctx->message_length_low >> 32);
    length_arr4[1] = (uint32_t) ctx->message_length_high;
    length_arr4[0] = (uint32_t) (ctx->message_length_high >> 32);

    /* Add bit_length_low (with carry propagation) */
    tmp = (uint64_t) length_arr4[3] + (uint64_t) bit_length_low;
    length_arr4[3] = (uint32_t) tmp;
    tmp = (uint64_t) length_arr4[2] + (tmp >> 32);
    length_arr4[2] = (uint32_t) tmp;
    tmp = (uint64_t) length_arr4[1] + (tmp >> 32);
    length_arr4[1] = (uint32_t) tmp;
    tmp = (uint64_t) length_arr4[0] + (tmp >> 32);
    length_arr4[0] = (uint32_t) tmp;

    /* Add bit_length_high (with carry propagation) */
    tmp = (uint64_t) length_arr4[2] + (uint64_t) bit_length_high;
    length_arr4[2] = (uint32_t) tmp;
    tmp = (uint64_t) length_arr4[1] + (tmp >> 32);
    length_arr4[1] = (uint32_t) tmp;
    tmp = (uint64_t) length_arr4[0] + (tmp >> 32);
    length_arr4[0] = (uint32_t) tmp;

    /* Write result to context */
    ctx->message_length_low = ((uint64_t) length_arr4[3]) + (((uint64_t) length_arr4[2]) << 32);
    ctx->message_length_high = ((uint64_t) length_arr4[1]) + (((uint64_t) length_arr4[0]) << 32);
}
