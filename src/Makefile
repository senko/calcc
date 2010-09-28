CC = gcc
CFLAGS = -Wall -O0 -g
# CFLAGS = -Wall -O3
LDFLAGS = -Wall -O0 -g

OBJS = emit.o input.o lex.o parse.o var.o func.o strings.o main.o

all:	calcc

calcc:	$(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o calcc
