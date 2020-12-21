# Bryan Bennett
# Simple make file
# 10/26/2020
# CSC 250 Lab 5

SOURCE = lab5.c
EXECUTABLE = lab5
COMPILE_FLAGS = -ansi -pedantic -Wall -c -I.
HEADERS = ppm_read_write.h get_image_args.h
OBJECTS = lab5.o ppm_read_write.o get_image_args.o

$(EXECUTABLE): $(OBJECTS)
	gcc -o $@ $^

%.o: %.c $(HEADERS)
	gcc $(COMPILE_FLAGS) -o $@ $<

tidy: $(SOURCE)
	clang-tidy -checks='*' $(SOURCE) -- -std=c99

clean:
	rm -f *.o $(EXECUTABLE)
