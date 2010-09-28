#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "calc.h"

static FILE *out;

int emit_init(char *file)
{
	out = fopen(file, "w");
	if (!out) {
		perror("can't open output file");
		return -1;
	}
	return 0;
}

void emit_finish(void)
{
	fprintf(out, "\n");
	fclose(out);
}

void emit_variable(char *name)
{
	/* no-op */
}

void emit_function_start(char *name, int argc)
{
	int i;
	fprintf(out, "\n\nglobal %s:function\n%s:\n", name, name);
	fprintf(out, "\tpush ebp\n");
	fprintf(out, "\tmov ebp, esp\n");
	fprintf(out, "\tsub esp, %d\n", N_VARS * 4);
	for (i = 0; i < argc; i++) {
		fprintf(out, "\tmov eax, [ebp + %d]\n", 8 + i * 4);
		fprintf(out, "\tmov [ebp - %d], eax\n", 4 + i * 4);
	}
	fprintf(out, "\n");
}

void emit_function_end(void)
{
	fprintf(out, "\n\tmov eax, 0\n");
	fprintf(out, ".Lfe:\n");
	fprintf(out, "\tleave\n");
	fprintf(out, "\tret\n\n");
}

void emit_return(int idx)
{
	if (idx != -1) fprintf(out, "\tmov eax, [ebp - %d]\n", 4 + idx * 4);
	fprintf(out, "\tjmp .Lfe\n");
}

void emit_call_arg(int idx)
{
	fprintf(out, "\tmov eax, [ebp - %d]\n", 4 + idx * 4);
	fprintf(out, "\tpush eax\n");
}

void emit_call(char *name, int argc, int idx)
{
	fprintf(out, "\tcall %s\n", name);
	fprintf(out, "\tadd esp, %d\n", argc * 4);
	fprintf(out, "\tmov [ebp - %d], eax\n", 4 + idx * 4);
}

void emit_constant(int idx, long val)
{
/*	fprintf(out, "\tmov eax, %ld\n", val);
	fprintf(out, "\tmov [ebp - %d], eax\n", 4 + idx * 4); */
	fprintf(out, "\tmov [ebp - %d], long %ld\n", 4 + idx * 4, val);
}

static void emit_3c(char *op, int left, int right, int result)
{
	fprintf(out, "\tmov eax, [ebp - %d]\n", 4 + left * 4);
	fprintf(out, "\t%s eax, [ebp - %d]\n", op, 4 + right * 4);
	fprintf(out, "\tmov [ebp - %d], eax\n", 4 + result * 4);
}

void emit_mul(int left, int right, int result)
{
	emit_3c("imul", left, right, result);
}

void emit_div(int left, int right, int result)
{
	fprintf(out, "\txor edx, edx\n");
	fprintf(out, "\tmov eax, [ebp - %d]\n", 4 + left * 4);
	fprintf(out, "\tidiv long [ebp - %d]\n", 4 + right * 4);
	fprintf(out, "\tmov [ebp - %d], eax\n", 4 + result * 4);
}
void emit_mod(int left, int right, int result)
{
	fprintf(out, "\txor edx, edx\n");
	fprintf(out, "\tmov eax, [ebp - %d]\n", 4 + left * 4);
	fprintf(out, "\tidiv long [ebp - %d]\n", 4 + right * 4);
	fprintf(out, "\tmov [ebp - %d], edx\n", 4 + result * 4);
}
void emit_add(int left, int right, int result)
{
	emit_3c("add", left, right, result);
}
void emit_sub(int left, int right, int result)
{
	emit_3c("sub", left, right, result);
}

void emit_assign(int dest, int src)
{
	fprintf(out, "\tmov eax, [ebp - %d]\n", 4 + src * 4);
	fprintf(out, "\tmov [ebp - %d], eax\n", 4 + dest * 4);
}

void emit_label(int index)
{
	fprintf(out, ".L%d:\n", index);
}

void emit_test(int op, int left, int right, int label)
{
	char *opcode = "jmp";
	switch (op) {
		case LEX_EQ: opcode = "jz"; break;
		case LEX_NE: opcode = "jnz"; break;
		case LEX_LT: opcode = "jl"; break;
		case LEX_GT: opcode = "jg"; break;
		case LEX_LE: opcode = "jle"; break;
		case LEX_GE: opcode = "jge"; break;
	}
	fprintf(out, "\n\tmov eax, [ebp - %d]\n", 4 + left * 4);
	fprintf(out, "\tcmp eax, [ebp - %d]\n", 4 + right * 4);
	fprintf(out, "\t%s .L%d\n", opcode, label);
}

void emit_jump(int label)
{
	fprintf(out, "\tjmp .L%d\n", label);
}

void emit_extern(char *name)
{
	fprintf(out, "\nextern %s\n", name);
}

void emit_asciiz(int index, char *string)
{
	fprintf(out, "LC%d: db ", index);
	while (*string) fprintf(out, "%d, ", *string++);
	fprintf(out, "0\n");
}

void emit_pushref(int index)
{
	fprintf(out, "\tpush LC%d\n", index);
}

void emit_pushconst(int val)
{
	fprintf(out, "\tpush long %d\n", val);
}

