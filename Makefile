CC=gcc
CFLAGS=-I. -Wall -g -std=c11 `mysql_config --cflags --libs`
DEPS = csv.h sql_op.h
OBJ = libcsv.o main.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

parse_csv: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -rf ./*.o parse_csv

valgrind: parse_csv
	valgrind --leak-check=yes ./parse_csv ./accounting_sample.csv
