# ajasta Makefile
#
# To build cursetag, you require a Curses library (such as GNU ncurses).
# taglib. Change anything you need below. You might just be able to run 'make'.
# This produces the executable 'ajasta' in the same directory.
#
CFLAGS=-O2 -g0 -Wall -Wextra -ansi -pedantic -std=c99
#CFLAGS=-O0 -ggdb -Wall -Wextra

CC=gcc
RM=rm -f
LDLIBS=-lncurses

# The rest should not need to be modified:
#####################################################################

all: ajasta 

options:
	@echo "used CPPFLAGS = ${CPPFLAGS}"

ajasta:
	$(CC) -o ajasta ajasta.c $(CFLAGS) $(LDLIBS)

clean:
	$(RM) ajasta
