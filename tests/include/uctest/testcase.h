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

/*
typedef struct
{
    FILE *fhandle;
    char *line;
    size_t line_size;
} uc_test;

typedef struct
{
    char **data;
    int n;
} uc_testcase;

int uc_test_init(uc_test *t, const char *fpath);
int uc_test_parse_line(uc_test *t, const char *delim, uc_testcase *tc);
int uc_test_next(uc_test *t);



int uc_testcase_init(uc_testcase *tc, int n);

int uc_testcase_free(uc_testcase *tc);
 */

#endif //UCRYPT_TESTCASE_H

