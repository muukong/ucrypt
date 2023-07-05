#ifndef UCRYPT_PBKDF2_H
#define UCRYPT_PBKDF2_H

#include <stdint.h>
#include <stddef.h>
#include <ucrypt/sha.h>

/*
 * Supported pseudo random functions (PRF)
 */
typedef enum uc_pbkdf2_prf
{
    UC_PBKDF2_HMAC_SHA1,
    UC_PBKDF2_HMAC_SHA224,
    UC_PBKDF2_HMAC_SHA256,
    UC_PBKDF2_HMAC_SHA384,
    UC_PBKDF2_HMAC_SHA512,
} uc_pbkdf2_prf;

/*
 * PBKDF2 with HMAC-SHA256
 */
int uc_pbkdf2(uint8_t *password, int password_length, uint8_t *salt, int salt_length,
              int iter_count, int derived_key_length, uint8_t *derived_key);

/*
 * PBKDF2 with HMAC based on one of the supported SHA hash functions
 */
int uc_pbkdf2_with_prf(uc_pbkdf2_prf prf, uint8_t *password, int password_length, uint8_t *salt,
                       int salt_length, int iter_count, int derived_key_length, uint8_t *derived_key);

#endif //UCRYPT_PBKDF2_H
