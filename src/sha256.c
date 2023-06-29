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

#define word_t uint32_t


//const uc_byte_t masks[8] = {
//    0x00, /* 0 0b00000000 */
//    0x80, /* 1 0b10000000 */
//    0xC0, /* 2 0b11000000 */
//    0xE0, /* 3 0b11100000 */
//    0xF0, /* 4 0b11110000 */
//    0xF8, /* 5 0b11111000 */
//    0xFC, /* 6 0b11111100 */
//    0xFE  /* 7 0b11111111 */
//};

/*
 * Computes non-negative integer K such that L + 1 + K = 448 (mod 512)
 * where L is the length (in bits) of the message.
 */
int _uc_sha256_pad_length(uc_sha_256_ctx_t *ctx);

const word_t K[] = {
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

int uc_sha256_transform_block(uc_sha_256_ctx_t *ctx)
{
    int t, t4;
    uint32_t a, b, c, d, e, f, g, h, T1, T2;
    uint8_t *M;
    uint32_t *H, *W;

    H = ctx->H;
    M = ctx->block;
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

    return -1; // TODO
}

int uc_sha256_update(uc_sha_256_ctx_t *ctx, uint8_t *data, uint64_t nbytes)
{
    ctx->message_length += 8 * nbytes;

    while ( nbytes-- > 0 )
    {
        ctx->block[ctx->block_index++] = *data;

        if ( ctx->message_length == SHA256_MESSAGE_BLOCK_SIZE )
            uc_sha256_transform_block(ctx);

        ++data;
    }

    return -1; // TODO
}

int uc_sha256_update_final_bits(uc_sha_256_ctx_t *ctx, uint8_t data, uint64_t nbits)
{
    int block_idx;

    assert( nbits < 8 );

    //ctx->block[block_idx] = data; /* note: we might copy too much data here, but it will be overwritten during finalize */
    //ctx->message_length += nbits;

    return -1; // TODO
}

int uc_sha256_pad_message(uc_sha_256_ctx_t *ctx, uint8_t pad_byte)
{

    if ( ctx->block_index >= SHA256_MESSAGE_BLOCK_SIZE - 8)
    {
        ctx->block[ctx->block_index++] = pad_byte;
        while ( ctx->block_index < SHA256_MESSAGE_BLOCK_SIZE )
            ctx->block[ctx->block_index++] = 0;

        uc_sha256_transform_block(ctx);
    }
    else
    {
        ctx->block[ctx->block_index++] = pad_byte;
    }

    while ( ctx->block_index < SHA256_MESSAGE_BLOCK_SIZE - 8 )
        ctx->block[ctx->block_index++] = 0;

    ctx->block[56] = (uint8_t) (ctx->message_length >> 56);
    ctx->block[57] = (uint8_t) (ctx->message_length >> 48);
    ctx->block[58] = (uint8_t) (ctx->message_length >> 40);
    ctx->block[59] = (uint8_t) (ctx->message_length >> 32);
    ctx->block[60] = (uint8_t) (ctx->message_length >> 24);
    ctx->block[61] = (uint8_t) (ctx->message_length >> 16);
    ctx->block[62] = (uint8_t) (ctx->message_length >> 8);
    ctx->block[63] = (uint8_t) (ctx->message_length);

    uc_sha256_transform_block(ctx);

    return -1; // TODO
}

int uc_sha256_finalize(uc_sha_256_ctx_t *ctx, uint8_t pad_byte)
{
    int i;

    uc_sha256_pad_message(ctx, pad_byte);

    // TODO: clear message bytes

    ctx->message_length = 0;
    ctx->computed = 1;

    for ( i = 0; i < 8; ++i )
    {
        printf("H[%d] = %x\n", i, ctx->H[i]);
    }

}

//int uc_sha256_final(uc_sha_256_ctx_t *ctx)
//{
//    uint64_t i, K;
//    int block_idx, block_offset, bit_offset;
//    uint8_t *ptr;
//
////    uint8_t pad[8] = {
////            0x80,       /* 0 0b10000000 */
////            0x40,       /* 1 0b01000000 */
////            0x20,       /* 2 0b00100000 */
////            0x10,       /* 3 0b00010000 */
////            0x08,       /* 4 0b00001000 */
////            0x04,       /* 5 0b00000100 */
////            0x02,       /* 6 0b00000010 */
////            0x01,       /* 7 0b00000001 */
////    };
//
//    const uint32_t shifts[8] = {
//            31,
//            30,
//            29,
//            28,
//            27,
//            26,
//            25,
//            24
//    };
//
//    const uc_byte_t masks[8] = {
//        0x00, /* 0 0b00000000 */
//        0x80, /* 1 0b10000000 */
//        0xC0, /* 2 0b11000000 */
//        0xE0, /* 3 0b11100000 */
//        0xF0, /* 4 0b11110000 */
//        0xF8, /* 5 0b11111000 */
//        0xFC, /* 6 0b11111100 */
//        0xFE  /* 7 0b11111111 */
//    };
//
//    /* Number of '0' padding bits */
//    K = _uc_sha256_pad_length(ctx);
//    bit_offset = ctx->message_length % 8;
//    block_idx = ctx->block_length / 4;
//    block_offset = ctx->block_length % 4;
//
//    printf("K = %d\n", K);
//    printf("bit_offset = %d\n", bit_offset);
//    printf("block_idx = %d\n", block_idx);
//    printf("block_offset = %d\n", block_offset);
//
//    ctx->block[block_idx] &= masks[bit_offset];
//    ctx->block[block_idx] |= (1u << shifts[bit_offset]);
//    ctx->block_length += 4;
//    K -= 31;
//
//    if ( 8 * ctx->block_length == 512 )
//    {
//        uc_sha256_transform_block(ctx);
//        ctx->block_length = 0;
//    }
//
//    assert ( K % 8 == 0 );
//
//    for ( i = 0; i < K / 32; ++i )
//    {
//        block_idx = ctx->block_length / 4;
//        ctx->block[block_idx] = 0;
//        ctx->block_length += 4;
//
//        if ( 8 * ctx->block_length == 512 )
//        {
//            uc_sha256_transform_block(ctx);
//            ctx->block_length = 0;
//        }
//    }
//
//    printf("Message length = %d\n", ctx->message_length);
//    block_idx = ctx->block_length / 4;
//    ptr = ctx->block + block_idx;
//    ptr[0] = (ctx->message_length & 0xff00000000000000ul) >> 56;
//    ptr[1] = (ctx->message_length & 0x00ff000000000000ul) >> 48;
//    ptr[2] = (ctx->message_length & 0x0000ff0000000000ul) >> 40;
//    ptr[3] = (ctx->message_length & 0x000000ff00000000ul) >> 32;
//    ptr[4] = (ctx->message_length & 0x00000000ff000000ul) >> 24;
//    ptr[5] = (ctx->message_length & 0x0000000000ff0000ul) >> 16;
//    ptr[6] = (ctx->message_length & 0x000000000000ff00ul) >> 8;
//    ptr[7] = (ctx->message_length & 0x00000000000000fful);
//
//    printf("x = %llx\n", *((uint64_t *) ptr));
//
//    uc_sha256_transform_block(ctx);
//
//    for ( i = 0; i < 8; ++i )
//    {
//        printf("H[%d] = %x\n", i, ctx->H[i]);
//    }
//
//
//    return -1; // TODO
//}

int uc_sha256_init(uc_sha_256_ctx_t *ctx)
{
    int i;

    uint32_t *h = ctx->H;
    h[0] = 0x6a09e667;
    h[1] = 0xbb67ae85;
    h[2] = 0x3c6ef372;
    h[3] = 0xa54ff53a;
    h[4] = 0x510e527f;
    h[5] = 0x9b05688c;
    h[6] = 0x1f83d9ab;
    h[7] = 0x5be0cd19;

    //for ( i = 0; i < 16; ++i )
    //    ctx->block[i] = 0;

    ctx->block_index = 0;
    ctx->message_length = 0;
    ctx->computed = 0;

    return -1; // TODO
}

