# # Makefile for arith (Comp 40 Assignment 4)
# # 
# # Includes build rules for 40image
# #
# # 
# #
# # Last updated: March 08, 2022

############## Variables ###############

CC = gcc # The compiler being used

# Updating include path to use Comp 40 .h files and CII interfaces
IFLAGS = -I/comp/40/build/include -I/usr/sup/cii40/include/cii

CFLAGS = -g -std=gnu99 -Wall -Wextra -Werror -Wfatal-errors -pedantic $(IFLAGS)

# Linking flags
LDFLAGS = -g -L/comp/40/build/lib -L/usr/sup/cii40/lib64

# Libraries needed for linking
# All programs cii40 (Hanson binaries) and *may* need -lm (math)
LDLIBS = -lnetpbm -lcii40 -l40locality -larith40 -lm -lrt

INCLUDES = $(shell echo *.h)

############### Rules ###############

all: 40image


## Compile step (.c files -> .o files)

# To get *any* .o file, compile its .c file with the following rule.
%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $< -o $@


## Linking step (.o -> executable program)

40image: 40image.o uarray2.o uarray2b.o compress40.o compressions.o uarray2.o decompressions.o bitpack.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm -f 40image *.o

