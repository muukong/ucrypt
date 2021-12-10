#ifndef UCRYPT_PRIME_H
#define UCRYPT_PRIME_H

int uc_is_prime(uc_int *x, int *is_prime);

int uc_is_prime_trial_division(uc_int *x, int *is_prime);

#endif //UCRYPT_PRIME_H
