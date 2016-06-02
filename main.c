#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "csv.h"

static const char DELIMITER = ';';

struct parse_context {
    int y; // line index
    int x; // column index
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

char* num_idx_2_char_idx(int num)
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

void release_input_buf(char* buffer)
{
    free(buffer);
}

void end_of_field_cb(void* buf, size_t len, void* context_data)
{
    struct parse_context *context = (struct parse_context*) context_data;
    printf(
            "cell[%d][%s]=%*.*s\n", 
            context -> y + 1, 
            num_idx_2_char_idx(context -> x), 
            (int)len, 
            (int)len, 
            (char*)buf
            );
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
    context.y = 0;
    context.x = 0;
   
    file_buf_len = read_input(&file_buf);

    csv_init(&parser, CSV_STRICT);
//    csv_set_delim(&parser, DELIMITER);
    csv_parse(&parser, file_buf, file_buf_len, end_of_field_cb, end_of_record_cb, &context); 
    csv_free(&parser);

    release_input_buf(file_buf);
    return 0;
}
