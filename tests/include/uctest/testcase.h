#ifndef UCRYPT_TESTCASE_H
#define UCRYPT_TESTCASE_H

#include <stdio.h>

typedef struct
{
    FILE *fhandle;
    char *line;     /* Current line in CSV file */
    size_t line_size;
} uc_csv_testcase;

int uc_test_init(uc_csv_testcase *t, const char *fpath);
int uc_test_init_len(uc_csv_testcase *t, size_t line_len, const char *fpath);

int uc_testcase_free(uc_csv_testcase *t);

int uc_testcase_next(uc_csv_testcase *t);
int uc_testcase_col(uc_csv_testcase*t, int col, char *data, size_t data_len);

#endif //UCRYPT_TESTCASE_H

