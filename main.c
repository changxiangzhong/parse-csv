#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sql_op.h"
#include "csv.h"

static const char DELIMITER = ';';

#define COLUMN_NAME_LEN 128
typedef char (*column_name)[COLUMN_NAME_LEN];
#define LINE_NAME_LEN 32
typedef char (*line_name)[LINE_NAME_LEN];

struct parse_context {
    unsigned int y; // line index
    unsigned int x; // column index
    line_name ln_name;
    column_name *cl_names;
    size_t cl_name_len;
    size_t cl_name_cap;
    MYSQL *mysql;
    MYSQL_STMT *pstmt;
};

void init_parse_context(struct parse_context *context)
{
    size_t i = 0;
    context -> x = 0;
    context -> y = 0;
    context -> ln_name = calloc(sizeof(char), LINE_NAME_LEN);

    context -> cl_name_len = 0;
    // hard code the initial length to 32
    context -> cl_name_cap = 32;
    context -> cl_names = calloc(sizeof(column_name), context->cl_name_cap);

    for (; i < context -> cl_name_cap; i++) {
       context -> cl_names[i] = calloc(sizeof(char), COLUMN_NAME_LEN);
    }

    context -> mysql = mysql_init(NULL);

    if (context -> mysql == NULL) {
        fprintf(stderr, "%s\n", mysql_error(context->mysql));
        exit(1);
    }

    if (mysql_real_connect(context -> mysql, MYSQL_HOSTNAME, MYSQL_USERNAME, MYSQL_PASSWORD, MYSQL_SCHEMA, 0, NULL, 0) == NULL) {
        goto mysql_init_error;
    }

    if (mysql_query(context -> mysql, MYSQL_CREATE_TABLE)) {
        goto mysql_init_error;
    }

    if (mysql_autocommit(context -> mysql, 0)) {
        goto mysql_init_error;
    }

    if ((context -> pstmt = mysql_stmt_init(context -> mysql)) == NULL) {
        goto mysql_init_error;
    }

    if (mysql_stmt_prepare(context -> pstmt, MYSQL_INSERT, strlen(MYSQL_INSERT)) != 0) {
        goto mysql_init_error;
    }

    return;
mysql_init_error:
        fprintf(stderr, "%s\n", mysql_error(context->mysql));
        mysql_close(context -> mysql);
        exit(1);
}

void expand_parse_contet_1st_column(struct parse_context *context)
{
    size_t i;
    printf("expand_parse_contet_1st_column\n");
    column_name* tmp = context -> cl_names;// save the temp pointer
    context -> cl_name_cap *= 2;
    context -> cl_names = calloc(sizeof(column_name), context->cl_name_cap);
    memcpy(context -> cl_names, tmp, context -> cl_name_len * sizeof(column_name));

    free(tmp);

    for (i = context -> cl_name_len; i < context -> cl_name_cap; i++) {
       context -> cl_names[i] = calloc(sizeof(char), COLUMN_NAME_LEN);
    }
}

void free_parse_context(struct parse_context *context)
{
    size_t i = 0;
    free(context -> ln_name);

    for (; i < context -> cl_name_cap; i++) {
        free(context -> cl_names[i]);
    }
    free(context -> cl_names);
    mysql_commit(context-> mysql);
    mysql_stmt_close(context -> pstmt);
    mysql_close(context -> mysql);
}

/**
 * @buffer pointer to buffer
 * @return buffer size when @return >= 0, else error
 */
long read_input(char** buffer)
{
    char file_name[] = "accounting_sample.csv";
    FILE *f = fopen(file_name, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    printf("size_of(%s) = %ld\n", file_name, fsize);
    fseek(f, 0, SEEK_SET);

    *buffer = malloc(fsize);
    fread(*buffer, fsize, 1, f);
    fclose(f);

    return fsize;
}

char* to_alphabetic_column(int num)
{
#define RET_BUF_LEN 32
    static char ret[RET_BUF_LEN];
    int quotient;// MS Office use A-Z to represent column
    int remainder = num;

    int i = 0;
    memset (ret, 0, RET_BUF_LEN);

    do {
        quotient = remainder / 26;
        remainder = remainder - quotient * 26;

        if (quotient > 0) 
            ret[i++] = 'A' + quotient - 1;

    } while (remainder >= 26);

    ret[i++] = 'A' + remainder;

    return ret;
}

void free_input_buf(char* buffer)
{
    free(buffer);
}

void end_of_field_cb(void* buf, size_t len, void* context_data)
{
    struct parse_context *context = (struct parse_context*) context_data;
    double value;
    char *str, *endptr;
    MYSQL_TIME time;
    MYSQL_BIND param[3];// For this case it's 3 params;
    
    memset(param, 0, sizeof param);

    str = calloc(sizeof(char), len + 1);
    strncpy(str, buf, len);

    value = strtof(str, &endptr);
    printf("str = %lu, len = %d, endptr = %lu\n", (unsigned long) str, (int)len, (unsigned long) endptr);
    
    if (len == 0 || endptr != str + len) {// Either empty value OR conversion failure
#define FIELD_VALUE_READ_FAILURE -1
        value = FIELD_VALUE_READ_FAILURE;
    }

    if (context -> y == 0) {// Store the 1st column names
        if (context -> x >= context -> cl_name_cap) {// expand it if necessary 
            expand_parse_contet_1st_column(context);
        }

        // save the 1st column items as name
        strncpy((char*) context -> cl_names[context -> cl_name_len++], buf, len);
    }

    if (context -> x == 0) {
        strncpy((char*)context -> ln_name, buf, len);
    } else {
        printf(
            "@year=%s\t@company=%s\tcell[%d][%s]=%*.*s(%f)\n", 
            (char*)context -> ln_name,
            (char*)context -> cl_names[context -> x],
            context -> y + 1, 
            to_alphabetic_column(context -> x), 
            (int)len, 
            (int)len, 
            (char*)buf,
            value
            );

        if (context -> y > 0) {
            // TODO: examine if it's convertable 
            param[0].buffer_type = MYSQL_TYPE_STRING;
            param[0].buffer = context -> cl_names[context -> x];
            param[0].buffer_length = strlen(context -> cl_names[context -> x]); 

            time.year = atoi((char*)context -> ln_name);
            time.month = 1;
            time.day = 1;
            param[1].buffer_type = MYSQL_TYPE_DATE;
            param[1].buffer = &time;

            param[2].buffer_type = MYSQL_TYPE_DOUBLE;
            param[2].buffer = &value;

            if (mysql_stmt_bind_param(context -> pstmt, param) != 0) {
                fprintf(stderr, " mysql_stmt_bind_param() failed\n");
                fprintf(stderr, " %s\n", mysql_stmt_error(context -> pstmt));
                exit(0);
            }

            if (mysql_stmt_execute(context -> pstmt) != 0) {
                fprintf(stderr, " mysql_stmt_execute(), 1 failed\n");
                fprintf(stderr, " %s\n", mysql_stmt_error(context -> pstmt));
                exit(0);
            }
        }
    }

    free(str);
    context -> x++;
}

void end_of_record_cb(int end_char, void* context_data)
{
    printf("file_read_callback() end_char=%d\n", end_char);
    struct parse_context *context = (struct parse_context*) context_data;
    context -> y++;

    // moved to the next line, reset curr_idx
    context -> x = 0;
}


int main(const int argc, const char const* argv[])
{
    struct csv_parser parser;
    char* file_buf;
    long file_buf_len;
    struct parse_context context;
    init_parse_context(&context);
   
    file_buf_len = read_input(&file_buf);

    csv_init(&parser, CSV_STRICT);
//    csv_set_delim(&parser, DELIMITER);
    csv_parse(&parser, file_buf, file_buf_len, end_of_field_cb, end_of_record_cb, &context); 
    csv_free(&parser);

    free_input_buf(file_buf);

    free_parse_context(&context);
    return 0;
}
