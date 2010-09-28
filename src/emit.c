#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "calc.h"

static FILE *out;
static char argc;
static int eax_var;

static char *loc(int index)
{
	static char location[128];
	if (index >= argc)
		sprintf(location, "[ebp - %d]", 4 * (1 + index - argc));
	else
		sprintf(location, "[ebp + %d]", 4 * (2 + index));
	return strdup(location);
}

static int in_eax(int index) { return (eax_var == index); }
static void invalidate(int index) { if (eax_var == index) eax_var = -1; }
static void set_eax(int index) { invalidate(index); eax_var = index; }

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

void emit_function_start(char *name, int fargc)
{
	fprintf(out, "\n\nglobal %s:function\n%s:\n", name, name);
	fprintf(out, "\tpush ebp\n");
	fprintf(out, "\tmov ebp, esp\n");
	fprintf(out, "\tsub esp, %d\n\n", N_VARS * 4);
	argc = fargc; eax_var = -1; 
}

void emit_function_end(void)
{
	fprintf(out, ".Lfe:\n");
	fprintf(out, "\tleave\n");
	fprintf(out, "\tret\n\n");
}

void emit_return(int idx)
{
	char *a = loc(idx);
	if (idx != -1) {
		if (!in_eax(idx)) fprintf(out, "\tmov eax, %s\n", a); 
	}
	free(a);
	fprintf(out, "\tjmp .Lfe\n");
}

void emit_call_arg(int idx)
{
	char *a = loc(idx);
	if (in_eax(idx))
		fprintf(out, "\tpush eax\n");
	else
		fprintf(out, "\tpush long %s\n", a);
	free(a);
}

void emit_call(char *name, int fargc, int idx)
{
	char *a = loc(idx);
	fprintf(out, "\tcall %s\n", name);
	if (fargc > 0) fprintf(out, "\tadd esp, %d\n", fargc * 4);
	fprintf(out, "\tmov %s, eax\n", a); free(a);
	set_eax(idx);
}

void emit_constant(int idx, long val)
{
	char *a = loc(idx);
	fprintf(out, "\tmov %s, long %ld\n", a, val); free(a);
}

static void emit_3c(char *op, int left, int right, int result)
{
	char *a, *b, *c;
	a = loc(left); b = loc(right); c = loc(result);
	if (!in_eax(left)) fprintf(out, "\tmov eax, %s\n", a); free(a);
	fprintf(out, "\t%s eax, %s\n", op, b); free(b);
	fprintf(out, "\tmov %s, eax\n", c); free(c);
	set_eax(result);
}

static void emit_3float(char *op, int left, int right, int result)
{
	char *a, *b, *c;
	char *lm = "fld";
	char *iop = "";

	a = loc(left); b = loc(right); c = loc(result);

	if (var_type(left) == VAR_INT) lm = "fild";
	if (var_type(right) == VAR_INT) iop = "i";

	fprintf(out, "\t; floating point '%s': %s & %s -> %s\n", op, a, b, c);
	fprintf(out, "\t%s long %s\n", lm, a); free(a);
	fprintf(out, "\tf%s%s long %s\n", iop, op, b); free(b);
	fprintf(out, "\tfstp long %s\n", c); free(c);
	invalidate(result);
}

void emit_mul(int left, int right, int result)
{
	if ((var_type(left) == VAR_FLOAT) || (var_type(right) == VAR_FLOAT)) {
		emit_3float("mul", left, right, result);
		return;
	}
	emit_3c("imul", left, right, result);
}

static void emit_divop(char *reg, int left, int right, int result)
{
	char *a, *b, *c;
	a = loc(left); b = loc(right); c = loc(result);
	fprintf(out, "\txor edx, edx\n");
	if (!in_eax(left)) fprintf(out, "\tmov eax, %s\n", a); free(a);
	fprintf(out, "\tidiv long %s\n", b); free(b);
	fprintf(out, "\tmov %s, %s\n", c, reg); free(c);
	invalidate(result); eax_var = -1;
}

void emit_div(int left, int right, int result)
{
	if ((var_type(left) == VAR_FLOAT) || (var_type(right) == VAR_FLOAT)) {
		emit_3float("div", left, right, result);
		return;
	}
	emit_divop("eax", left, right, result);
	set_eax(result);
}

void emit_mod(int left, int right, int result)
{
	if ((var_type(left) == VAR_FLOAT) || (var_type(right) == VAR_FLOAT)) {
		printf("[panic] can't use modulo operation for floats\n");
		exit(EXIT_FAILURE);
	}
	emit_divop("edx", left, right, result);
}
void emit_add(int left, int right, int result)
{
	if ((var_type(left) == VAR_FLOAT) || (var_type(right) == VAR_FLOAT)) {
		emit_3float("add", left, right, result);
		return;
	}
	emit_3c("add", left, right, result);
}
void emit_sub(int left, int right, int result)
{
	if ((var_type(left) == VAR_FLOAT) || (var_type(right) == VAR_FLOAT)) {
		emit_3float("sub", left, right, result);
		return;
	}
	emit_3c("sub", left, right, result);
}

void emit_label(int index)
{
	fprintf(out, ".L%d:\n", index);
	eax_var = -1; /* start or end of basic block */
}

void emit_test(int op, int left, int right, int label)
{
	char *a, *b; char *opcode = "jmp";
	a = loc(left); b = loc(right);
	switch (op) {
		case LEX_EQ: opcode = "jz"; break;
		case LEX_NE: opcode = "jnz"; break;
		case LEX_LT: opcode = "jl"; break;
		case LEX_GT: opcode = "jg"; break;
		case LEX_LE: opcode = "jle"; break;
		case LEX_GE: opcode = "jge"; break;
	}
	if ((var_type(left) == VAR_FLOAT) && (var_type(right) == VAR_INT)) {
		fprintf(out, "\tf%sld long %s\n",
			(var_type(left) == VAR_INT) ? "i" : "", a);
		fprintf(out, "\tf%scom long %s\n",
			(var_type(right) == VAR_INT) ? "i" : "", a);
	} else {
		if (!in_eax(left)) fprintf(out, "\tmov eax, %s\n", a);
		fprintf(out, "\tcmp eax, %s\n", b);
	}
	free(a); free(b);
	fprintf(out, "\t%s .L%d\n", opcode, label);
}

void emit_jump(int label)
{
	fprintf(out, "\tjmp .L%d\n", label);
}

void emit_extern(char *name)
{
	fprintf(out, "extern %s\n", name);
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

void emit_cast_f2i(int dest, int src)
{
	char *a; char *b; a = loc(src); b = loc(dest);
	fprintf(out, "\t; float -> integer : load, round, store\n");
	fprintf(out, "\tfld long %s\n", a); free(a);
	fprintf(out, "\tfrndint\n");
	fprintf(out, "\tfistp long %s\n", b); free(b);
	invalidate(dest);
}

void emit_cast_i2f(int dest, int src)
{
	char *a; char *b; a = loc(src); b = loc(dest);
	fprintf(out, "\t; integer -> float : load, store\n");
	fprintf(out, "\tfild long %s\n", a); free(a);
	fprintf(out, "\tfstp long %s\n", b); free(b);
	invalidate(dest);
}

void emit_assign(int dest, int src)
{
	char *a, *b;

	if ((var_type(dest) == VAR_FLOAT) && (var_type(src) == VAR_INT))
		{ emit_cast_i2f(dest, src); return; }
	else if ((var_type(src) == VAR_FLOAT) && (var_type(dest) == VAR_INT))
		{ emit_cast_f2i(dest, src); return; }

	a = loc(src); b = loc(dest);
	if (!in_eax(src)) fprintf(out, "\tmov eax, %s\n", a); free(a);
	fprintf(out, "\tmov %s, eax\n", b); free(b);
	set_eax(dest);
}

