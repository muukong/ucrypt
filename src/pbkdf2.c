#include <memory.h>
#include <ucrypt/pbkdf2.h>

/*
 * PBKDF2 implementation as specified in https://www.ietf.org/rfc/rfc2898.txt
 */

#define DIV_CEIL(x, y) (1 + (((x) - 1) / (y)));

int _uc_pbkdf2(uc_sha_hmac_ctx_t  *hmac, uint8_t *salt, int salt_length,
               int iter_count, int derived_key_length, uint8_t *derived_key);

/*
 * Implements function F as specified in https://www.ietf.org/rfc/rfc2898.txt (see section 5.2)
 */
int _F(uc_sha_hmac_ctx_t *hmac, uint8_t *salt, int salt_length, int c, int i, uint8_t *F, int F_length);

int uc_pbkdf2(uint8_t *password, int password_length, uint8_t *salt, int salt_length,
              int iter_count, int derived_key_length, uint8_t *derived_key)
{
    int res;
    uc_sha_hmac_ctx_t hmac_sha256;

    if ( (res = uc_sha_hmac_init(&hmac_sha256, UC_SHA256, password, password_length)) != UC_OK )
        return res;

    return _uc_pbkdf2(&hmac_sha256, salt, salt_length, iter_count, derived_key_length, derived_key);

}

int uc_pbkdf2_with_prf(uc_pbkdf2_prf prf, uint8_t *password, int password_length, uint8_t *salt, int salt_length,
              int iter_count, int derived_key_length, uint8_t *derived_key)
{
    int res;
    uc_sha_hmac_ctx_t hmac;

    switch ( prf )
    {
        case UC_PBKDF2_HMAC_SHA224:
            res = uc_sha_hmac_init(&hmac, UC_SHA224, password, password_length);
            break;
        case UC_PBKDF2_HMAC_SHA256:
            res = uc_sha_hmac_init(&hmac, UC_SHA256, password, password_length);
            break;
        case UC_PBKDF2_HMAC_SHA384:
            res = uc_sha_hmac_init(&hmac, UC_SHA384, password, password_length);
            break;
        case UC_PBKDF2_HMAC_SHA512:
            res = uc_sha_hmac_init(&hmac, UC_SHA512, password, password_length);
            break;
        default:
            return UC_INPUT_ERR;
    }

    if ( res != UC_OK )
        return res;

    return _uc_pbkdf2(&hmac, salt, salt_length, iter_count, derived_key_length, derived_key);
}

int _uc_pbkdf2(uc_sha_hmac_ctx_t  *hmac, uint8_t *salt, int salt_length,
               int iter_count, int derived_key_length, uint8_t *derived_key)
{
    int res;
    int i;
    int l;              /* number hash_length-octet blocks in derived key */
    int r;

    l = DIV_CEIL(derived_key_length, hmac->sha_digest_length);
    r = derived_key_length - (l - 1) * hmac->sha_digest_length;

    /* Calculate blocks T_1, T_2, ..., T_l */
    for ( i = 1; i <= l; ++i )
    {

        uc_sha_hmac_reset(hmac); /* function _F expects a "fresh" HMAC context */

        if ( i == l ) /* last block */
        {
            /* only copy r octets of T_i into the derived key buffer */
            if ( (res = _F(hmac, salt, salt_length, iter_count, i, derived_key + (i - 1)* hmac->sha_digest_length, r)) != UC_OK )
                return res;
        }
        else
        {
            if ( (res = _F(hmac, salt, salt_length, iter_count, i, derived_key + (i - 1) * hmac->sha_digest_length, hmac->sha_digest_length)) != UC_OK )
                return res;
        }

    }

    return UC_OK;
}

int _F(uc_sha_hmac_ctx_t *hmac, uint8_t *salt, int salt_length, int c, int i, uint8_t *F, int F_length)
{
    int res;
    int t, t2;
    uint8_t U[UC_SHA_HMAC_MAX_LENGTH];
    uint8_t i_octets[4];

    /* Write i to i_octets (most significant octet first) */
    i_octets[0] = (uint8_t) ((uint32_t) i >> 24);
    i_octets[1] = (uint8_t) ((uint32_t) i >> 16);
    i_octets[2] = (uint8_t) ((uint32_t) i >> 8);
    i_octets[3] = (uint8_t) i;

    /* Compute U_1 = PRF(P, S || INT(i))  and write to F */
    if ( (res = uc_sha_hmac_update(hmac, salt, salt_length)) != UC_OK ||
         (res = uc_sha_hmac_update(hmac, i_octets, 4)) != UC_OK ||
         (res = uc_sha_hmac_finalize(hmac)) != UC_OK ||
         (res = uc_sha_hmac_output(hmac, U)) != UC_OK )
    {
        return res;
    }

    /* F = U_1 */
    memcpy(F, U, F_length);

    /* Compute U_2, U_3, ..., U_c and XOR each with F */
    for ( t = 2; t <= c; ++t )
    {
        /* Rest HMAC context */
        if ( (res = uc_sha_hmac_reset(hmac)) != UC_OK )
            return res;

        /* Compute U_t = PRF(P, U_t) */
        if ( (res = uc_sha_hmac_update(hmac, U, hmac->sha_digest_length)) != UC_OK ||
             (res = uc_sha_hmac_finalize(hmac)) != UC_OK ||
             (res = uc_sha_hmac_output(hmac, U)) != UC_OK )
        {
            return res;
        }

        /* F = F XOR U */
        for ( t2 = 0; t2 < F_length; ++t2 )
            F[t2] ^= U[t2];
    }

    return UC_OK;
}
