CC = ../src/calcc
AS = nasm
LD = ld
AR = ar
RANLIB = ranlib

OBJS = crt.o library.o

all:	libcalc.a

libcalc.a: $(OBJS)
	ar cru $@ $(OBJS)
	ranlib $@

%.o:	%.asm
	nasm -f elf $< -o $@

%.asm:	%.calc
	$(CC) $< $@

clean:
	rm -f $(OBJS) libcalc.a library.asm


