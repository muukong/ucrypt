//
// Created by urs on 6/22/23.
//

#ifndef UCRYPT_RSA_H
#define UCRYPT_RSA_H

#include <ucrypt/integer.h>

struct uc_rsa_pub_key_t
{
    uc_int e;   /* public exponent */
    uc_int n;   /* modulus */
};

struct uc_rsa_priv_key_t
{
    uc_int d;
    uc_int n;
};

int uc_rsa_init_pub_key(struct uc_rsa_pub_key_t *pub_key);
int uc_rsa_init_priv_key(struct uc_rsa_priv_key_t *priv_key);
int uc_rsa_free_pub_key(struct uc_rsa_pub_key_t *pub_key);
int uc_rsa_free_priv_key(struct uc_rsa_priv_key_t *priv_key);

int uc_rsa_gen_key(struct uc_rsa_pub_key_t *pub_key, struct uc_rsa_priv_key_t *priv_key, int nbits);

int uc_rsa_encrypt(uc_int *c, uc_int *m, struct uc_rsa_pub_key_t *pub_key);
int uc_rsa_decrypt(uc_int *m, uc_int *c, struct uc_rsa_priv_key_t *priv_key);

#endif //UCRYPT_RSA_H
