#ifndef UCRYPT_PRIME_H
#define UCRYPT_PRIME_H

int uc_is_prime(uc_int *x, int *is_prime, int safe);

int uc_is_prime_trial_division(uc_int *x, int *is_prime);
int uc_is_prime_miller_rabin(uc_int *n, int *is_prime, int t);
int gen_rand_prime(uc_int *x, uc_int *a, uc_int *b);

#endif //UCRYPT_PRIME_H