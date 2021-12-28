#include <stdlib.h>
#include <string.h>
#include <ucrypt/string_util.h>
#include <uctest/testcase.h>

#define DEFAULT_LINE_LEN (4096 * 8)

int uc_test_init(uc_csv_testcase *t, const char *fpath)
{
    return uc_test_init_len(t, DEFAULT_LINE_LEN, fpath);
}

int uc_test_init_len(uc_csv_testcase *t, size_t line_len, const char *fpath)
{
    if ( !(t->fhandle = fopen(fpath, "r")) )
        return 0;

    if ( !(t->line = malloc(line_len)) )
    {
        fclose(t->fhandle);
        t->fhandle = NULL;
        t->line = NULL;
        t->line_size = 0;
        return 0;
    }

    t->line_size = line_len;
    return 1;
}

int uc_testcase_free(uc_csv_testcase *t)
{
    if ( t->fhandle )
        fclose(t->fhandle);
    if ( t->line )
        free(t->line);

    t->fhandle = NULL;
    t->line = NULL;
    t->line_size = 0;

    return 1;
}

int uc_testcase_next(uc_csv_testcase *t)
{
    if ( !fgets(t->line, t->line_size, t->fhandle) )
        return 0;

    return 1;
}

int uc_testcase_col(uc_csv_testcase*t, int col, char *data, size_t data_len)
{
    int res;
    char *line, *tok;

    res = 1;

    line = strdup(t->line);

    for ( tok = strtok(line, ","); tok && *tok; tok = strtok(NULL, ","))
    {
        if ( col == 0 )
            break;
        --col;
    }

    /* Error if there are fewer columns in the data than requested or if data cannot hold result */
    if ( col != 0 || strlen(line) > data_len )
    {
        res = 0;
        goto cleanup;
    }

    remove_whitespace(tok);
    strncpy(data, tok, data_len + 1);

cleanup:
    free(line);

    return res;
}

/*
#define LINE_BUF_INITIAL_SIZE (8 * 4096)

int uc_test_init(uc_test *t, const char *fpath)
{
    //puts(MUL_DATA_DIR) ;

    if ( !(t->fhandle = fopen(fpath, "r")) )
        return 0;

    if ( !(t->line = malloc(LINE_BUF_INITIAL_SIZE)) )
    {
        fclose(t->fhandle);
        return 0;
    }
    t->line_size = LINE_BUF_INITIAL_SIZE;

    return 1;
}

int uc_test_close(uc_test *t)
{
    fclose(t->fhandle);
    t->fhandle = NULL;

    free(t->line);
    t->line = NULL;
    t->line_size = 0;

    return 1;
}

int uc_testcase_init(uc_testcase *tc, int n)
{
    int i;

    tc->data = malloc(n);
    for ( i = 0; i < n; ++i )
        tc->data[i] = 0;
    tc->n = n;

    return 1;
}

int uc_testcase_free(uc_testcase *tc)
{
    int i;

    for ( i = 0; i < tc->n; ++i )
    {
        free(tc->data[i]);
        tc->data[i] = 0;
    }

    free(tc->data);
    tc->n = 0;

    return 1;
}

int uc_test_parse_line(uc_test *t, const char *delim, uc_testcase *tc)
{
    int i, ncols;
    char *tmp;
    char *line, *tok;

    ncols = 1; // a row contains at least one entry
    for (tmp = t->line; *tmp != '\0'; ++tmp )
    {
        if (*tmp == *delim)
            ++ncols;
    }

    uc_testcase_init(tc, ncols);

    line = strdup(t->line);
    i = 0;
    for ( tok = strtok(line, ","); tok && *tok; tok = strtok(NULL, ",\n"))
    {
        remove_whitespace(tok);
        puts(tok);
        tc->data[i] = malloc(strlen(tok));
        strcpy(tc->data[i], tok);
        ++i;
    }
    free(line);

    return 1;
}

int uc_test_next(uc_test *t)
{
    // read next lien and discard newline character
    if ( !fgets(t->line, t->line_size, t->fhandle) )
        return 0;

    return 1;
}
*/
