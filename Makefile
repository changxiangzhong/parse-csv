CC=gcc
CFLAGS=-I. -Wall
DEPS = csv.h
OBJ = libcsv.o main.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

parse_csv: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -rf ./*.o parse_csv
