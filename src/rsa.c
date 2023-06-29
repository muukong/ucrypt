//
// Created by urs on 6/22/23.
//

#include <assert.h>
#include <stddef.h>
#include <ucrypt/rsa.h>
#include <ucrypt/rand.h>



int uc_rsa_init_pub_key(struct uc_rsa_pub_key_t *pub_key)
{
    return uc_init_multi(&pub_key->e, &pub_key->n, NULL, NULL, NULL, NULL);
}

int uc_rsa_init_priv_key(struct uc_rsa_priv_key_t *priv_key)
{
    return uc_init_multi(&priv_key->d, &priv_key->n, NULL, NULL, NULL, NULL);
}

int uc_rsa_free_pub_key(struct uc_rsa_pub_key_t *pub_key)
{
    return uc_free_multi(&pub_key->e, &pub_key->n, NULL, NULL, NULL, NULL);
}

int uc_rsa_free_priv_key(struct uc_rsa_priv_key_t *priv_key)
{
    return uc_free_multi(&priv_key->d, &priv_key->n, NULL, NULL, NULL, NULL);
}

int uc_rsa_gen_key(struct uc_rsa_pub_key_t *pub_key, struct uc_rsa_priv_key_t *priv_key, int nbits)
{
    int res, nbits_factor;
    uc_int a, b, p, q, n, phi;


    if ( (res = uc_init_multi(&a, &b, &p, &q, &n, &phi)) != UC_OK )
        return res;

    /* Generate primes p, q in range [ 2**((message_length/2)-1), 2**(message_length/2) ) */
    nbits_factor = nbits / 2;
    if ( (res = uc_set_d(&a, 1)) != UC_OK ||
         (res = uc_set_d(&b, 1)) != UC_OK ||
         (res = uc_lshb(&a, &a, nbits_factor - 1)) != UC_OK ||
         (res = uc_lshb(&b,&b, nbits_factor)) != UC_OK ||
         (res = uc_gen_rand_prime(&p, &a, &b)) != UC_OK ||
         (res = uc_gen_rand_prime(&q, &a, &b)) != UC_OK )
    {
        goto cleanup;
    }

    /* Compute modulus n */
    if ( (res = uc_mul(&n, &p, &q)) != UC_OK )
        goto cleanup;

    /*
     * Compute phi where
     *  a = p - 1
     *  b = q - 1
     *  phi = a * b = (p - 1) * (q - 1)
     */
    if ( (res = uc_copy(&a, &p)) != UC_OK ||
         (res = uc_sub_d(&a, &a, 1)) != UC_OK ||
         (res = uc_copy(&b, &q)) != UC_OK ||
         (res = uc_sub_d(&b, &b, 1)) != UC_OK ||
         (res = uc_mul(&phi, &a, &b)) != UC_OK )
    {
        goto cleanup;
    }

    /* Initialize public key */

    if ( (res = uc_set_d(&pub_key->e, (1 << 16) + 1)) != UC_OK ||
         (res = uc_copy(&pub_key->n, &n)) != UC_OK )
    {
        goto cleanup;
    }

    /* Initialize private key */
    if ( (res = uc_mod_inv(&priv_key->d, &pub_key->e, &phi)) != UC_OK ||
         (res = uc_copy(&priv_key->n, &n)) != UC_OK )
    {
        goto cleanup;
    }

cleanup:
    uc_free_multi(&a, &b, &p, &q, &n, &phi);

    return res;
}

int uc_rsa_encrypt(uc_int *c, uc_int *m, struct uc_rsa_pub_key_t *pub_key)
{
    assert(uc_is_neg(m) == UC_FALSE);
    assert(uc_lt(m, &pub_key->n) == UC_TRUE);

    return uc_exp_mod(c, m, &pub_key->e, &pub_key->n);
}

int uc_rsa_decrypt(uc_int *m, uc_int *c, struct uc_rsa_priv_key_t *priv_key)
{
    assert(uc_is_neg(c) == UC_FALSE);
    assert(uc_lt(c, &priv_key->n) == UC_TRUE);

    return uc_exp_mod(m, c, &priv_key->d, &priv_key->n);
}
