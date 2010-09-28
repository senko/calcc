CC = ../src/calcc
LDFLAGS = -L../lib -lcalc
AS = nasm
LD = ld

PROGRAMS = hello fib fact

all:	$(PROGRAMS)
	@echo "Build done. Try out test programs: $(PROGRAMS)"

hello:	hello.o
	ld $< -o $@ $(LDFLAGS)

fib:	fib.o
	ld $< -o $@ $(LDFLAGS)

fact:	fact.o
	ld $< -o $@ $(LDFLAGS)

%.o:	%.asm
	nasm -f elf $< -o $@

%.asm:	%.calc
	$(CC) $< $@

clean:
	rm -f $(PROGRAMS) *.o *.asm

