#ifndef UCRYPT_RAND_H
#define UCRYPT_RAND_H

int uc_rand_bytes(void *buf, int n);

int uc_rand_int(uc_int *x, uc_int *max);
int uc_rand_int_range(uc_int *x, uc_int *a, uc_int *b);
int uc_rand_int_base_pow(uc_int *x, int k);

int uc_rand_i(int *x);
int uc_rand_l(long *x);
int uc_rand_digit(uc_digit *d);
int uc_gen_rand_prime(uc_int *x, uc_int *a, uc_int *b);

#endif //UCRYPT_RAND_H
