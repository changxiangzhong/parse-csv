#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "csv.h"

static const char DELIMITER = ';';

struct parse_context {
    int field_cnt;
    int record_cnt;
};
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

void release_input_buf(char* buffer)
{
    free(buffer);
}

void end_of_field_cb(void* buf, size_t len, void* context_data)
{
    printf("field_read_callback() buf=%*.*s\n\n", (int)len, (int)len, (char*)buf);
    struct parse_context *context = (struct parse_context*) context_data;
    context -> field_cnt++;
}

void end_of_record_cb(int end_char, void* context_data)
{
    printf("file_read_callback() end_char=%d\n", end_char);
    struct parse_context *context = (struct parse_context*) context_data;
    context -> record_cnt++;
}


int main(const int argc, const char const* argv[])
{
    struct csv_parser parser;
    char* file_buf;
    long file_buf_len;
    struct parse_context context;
    context.field_cnt = 0;
    context.record_cnt = 0;
   
    file_buf_len = read_input(&file_buf);

    csv_init(&parser, CSV_STRICT);
    csv_set_delim(&parser, DELIMITER);
    csv_parse(&parser, file_buf, file_buf_len, end_of_field_cb, end_of_record_cb, &context); 
    csv_free(&parser);

    printf("==================\ncontext.field_cnt = %d\ncontext.record_cnt = %d\n==================\n",
            context.field_cnt,
            context.record_cnt
            );
    release_input_buf(file_buf);
    return 0;
}
