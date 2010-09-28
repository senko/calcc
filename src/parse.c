#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "calc.h"

#define AUTOCAST(x, l, r) \
	if ((var_type(l) == VAR_FLOAT) || (var_type(r) == VAR_FLOAT)) \
		var_set_type(x, VAR_FLOAT);

/*
 * program ::= fdecl | fdecl program
 *
 * line ::= cmd ';'
 * cmd ::= 'var' variable
 *	'return' expr
 *	'input' variable (not implemented)
 *	'print' variable (not implemented)
 *	variable '=' expr
 * test_exr ::= expr ==|!=|>|<|>=|<= expr
 * expr ::= expr +|- factor
 * factor ::= factor *|/ term
 * term ::= '(' expr ')' | variable | variable '(' [args] ')' | number
 * args ::= expr | expr ',' args
 * fdecl ::= 'function' variable '(' params ')' '{' body '}'
 * params ::= variable | variable ',' params
 */

static int parse_term(void);
static int parse_factor(void);
static int parse_expr(void);
static int parse_var(void);
static int parse_cmd(void);
static int parse_fcall(char *name);
static int parse_if(void);
static int parse_while(void);
static int parse_for(void);

static int label_count;
static int vardecl_ok;
static int loop_next, loop_end;
static char f_ret_type;

static int parse_var(void)
{
	int x, idx;
	char type;

	if (!vardecl_ok) {
		printf("illegal variable declaration placement\n");
		return -1;
	}
	type = VAR_INT;
	if (lex_peek() == LEX_FVAR) type = VAR_FLOAT;

	lex_consume();

	x = lex_peek();
	if (x != LEX_IDENTIFIER) return -1;

	idx = var_alloc(strdup(lex_get_string()), type);
	if (idx == -1) return -1;
	printf("variable [%s] stored in slot %d\n", lex_get_string(), idx);

	emit_variable(lex_get_string());
	lex_consume();
	return 0;
}

static int parse_return(void)
{
	int idx;
	int cast;

	lex_consume();
	idx = -1;
	if (lex_peek() != ';') {
		idx = parse_expr(); if (idx == -1) return -1;
		if (var_type(idx) != f_ret_type) {
			cast = var_alloc(NULL, f_ret_type);
			emit_assign(cast, idx); var_free(idx);
			idx = cast;
		}
	}
	emit_return(idx);
	if (idx != -1) var_free(idx);
	return 0;
}

static int parse_assign(void)
{
	char *name;
	int left, right;

	name = strdup(lex_get_string()); lex_consume();
	if (lex_peek() == '(') {
		left = parse_fcall(name); if (left == -1) return -1;
		var_free(left); return 0;
	}

	left = var_get(name);
	if (left == -1) {
		printf("unknown variable %s\n", name);
		return -1;
	}

	free(name);

	if (lex_peek() != '=') return -1;
	lex_consume();

	right = parse_expr(); if (right == -1) return -1;
	emit_assign(left, right); var_free(right);
	return 0;
}

static int parse_continue(void)
{
	lex_consume();
	if (loop_next == -1) {
		printf("'continue' not inside a loop\n");
		return -1;
	}
	emit_jump(loop_next);
	return 0;
}

static int parse_break(void)
{
	lex_consume();
	if (loop_next == -1) {
		printf("'break' not inside a loop\n");
		return -1;
	}
	emit_jump(loop_end);
	return 0;
}

static int parse_print(void)
{
	int x;
	int len;

	lex_consume();
	if (lex_peek() == LEX_STRING) {
		printf("[parse_print] string '%s'\n", lex_get_string());
		x = string_decl(strdup(lex_get_string()));
		len = strlen(lex_get_string()) + 1;
		emit_pushconst(len);
		emit_pushref(x); x = var_alloc(NULL, 0);
		emit_pushconst(1);
		emit_call("write", 3, x); var_free(x);
		lex_consume();
	}
	printf("[parse_print] clean exit\n");
	return 0;
}

static int parse_cmd(void)
{
	int x;

	x = lex_peek();
	if (x == LEX_IVAR) return parse_var();
	if (x == LEX_FVAR) return parse_var();
	if (x == LEX_RETURN) return parse_return();
	if (x == LEX_CONTINUE) return parse_continue();
	if (x == LEX_BREAK) return parse_break();
	if (x == LEX_IDENTIFIER) return parse_assign();
	if (x == LEX_PRINT) return parse_print();
/*	if (x == LEX_INPUT) return parse_input(); */
	return -1;
}

static int parse_body_single(void)
{
	printf("[parse_body_single] enter with token %d\n", lex_peek());
	if (lex_peek() == ';') { lex_consume(); return 0; }
	if (lex_peek() == '}') return 0;

	if (lex_peek() == LEX_IF) return parse_if();
	if (lex_peek() == LEX_WHILE) return parse_while();
	if (lex_peek() == LEX_FOR) return parse_for();

	if (parse_cmd() == -1) return -1;
	if (lex_peek() != ';') return -1; lex_consume();
	return 0;
}

static int parse_body(void)
{
	if (lex_peek() != '{') {
		printf("[parse_body] single statement\n");
		return parse_body_single();
	}	
	printf("[parse_body] compound statement\n");
	lex_consume();
	while (1) {
		printf("[parse_body] running parse_body_single\n");
		if (parse_body_single()) return -1;
		if (lex_peek() == '}') break;
	}
	lex_consume();
	printf("[parse_body] clean exit\n");
	return 0;
}

static int parse_test_expr(void)
{
	int left, right;
	int x;
	int label;

	left = parse_expr(); if (left == -1) return -1;
	x = lex_peek();
	if ((x != LEX_NE) && (x != LEX_EQ) && (x != LEX_GT) &&
		(x != LEX_GE) && (x != LEX_LT) && (x != LEX_LE)) return -1;
	lex_consume();
	right = parse_expr(); if (right == -1) return -1;
	label = label_count++;
	emit_test(x, left, right, label);
	var_free(left); var_free(right);
	printf("test_expr clean exit\n");
	return label;
}


static int parse_expr(void)
{
	int left, right, idx;
	int x;

	left = parse_factor(); if (left == -1) return -1;

	x = lex_peek();
	while ((x == '+') || (x == '-')) {
		lex_consume();
		right = parse_factor();
		idx = var_alloc(NULL, VAR_INT); if (idx == -1) return -1;
		if (x == '+') emit_add(left, right, idx);
			else emit_sub(left, right, idx);
		AUTOCAST(idx, left, right);
		var_free(left); var_free(right);
		left = idx;
		x = lex_peek();
	}
	return left;
}

static int parse_factor(void)
{
	int left, right, idx;
	int x;

	left = parse_term(); if (left == -1) return -1;

	x = lex_peek();
	while ((x == '*') || (x == '/') || (x == '%')) {
		lex_consume();
		right = parse_term();
		idx = var_alloc(NULL, VAR_INT); if (idx == -1) return -1;
		if (x == '*') emit_mul(left, right, idx);
			else if (x == '/') emit_div(left, right, idx);
			else emit_mod(left, right, idx);
		AUTOCAST(idx, left, right);
		var_free(left); var_free(right);
		left = idx;
		x = lex_peek();
	}
	return left;
}

static int parse_term(void)
{
	int idx = 0;
	int x;
	char *name;

	x = lex_peek();
	switch (x) {
		case '(':
			lex_consume();
			idx = parse_expr(); if (idx == -1) return -1;
			if (lex_peek() != ')') {
				printf("parse error, paretnthesis mismatch\n");
				return -1;
			}
			lex_consume();
			return idx;
		case LEX_INTEGER:
			idx = var_alloc(NULL, VAR_INT);
			if (idx == -1) return -1;
			emit_constant(idx, lex_get_integer());
			lex_consume();
			return idx;
		case LEX_FLOAT:
			idx = var_alloc(NULL, VAR_FLOAT);
			if (idx == -1) return -1;
			emit_constant(idx, lex_get_integer());
			lex_consume();
			return idx;
		case LEX_IDENTIFIER:
			printf("variable term `%s'\n", lex_get_string());
			name = strdup(lex_get_string()); lex_consume();
			if (lex_peek() == '(') return parse_fcall(name);
			idx = var_get(name);
			if (idx == -1) {
				printf("unknown variable %s\n", name);
				return -1;
			}
			free(name);
			return idx;
	}
	printf("parse error, unexpected token (%d)\n", x);
	return -1;
}

static int parse_function(void)
{
	int x;
	int idx;
	int argc;
	char *name;
	char type = VAR_INT;
	int externf = 0;

	if (lex_peek() == LEX_EXTERN) { externf = 1; lex_consume(); }
	printf("externf: %d, token %d\n", externf, lex_peek());

	lex_consume();
	x = lex_peek(); if (x != LEX_IDENTIFIER) return -1;
	name = strdup(lex_get_string()); lex_consume();

	if (lex_peek() != '(') return -1; lex_consume();
	argc = 0;
	var_init();
	while (lex_peek() != ')') {
		if (argc) { if (lex_peek() != ',') return -1; lex_consume(); }
		if (lex_peek() != LEX_IDENTIFIER) return -1;
		idx = var_alloc(strdup(lex_get_string()), 0);
		if (idx == -1) return -1;
		argc++;
		lex_consume();
	}
	lex_consume();

	if (lex_peek() == ':') { /* type decl */
		lex_consume();
		if (lex_peek() == LEX_IVAR)
			type = VAR_INT;
		else if (lex_peek() == LEX_FVAR)
			type = VAR_FLOAT;
		else { printf("illegal function return type\n"); return -1; }
		lex_consume();
	}

	if (func_decl(name, argc, type)) return -1;
	if (lex_peek() == ';') { /* just a prototype decl */
		lex_consume();
		var_fini();
		if (externf) emit_extern(name);
		return 0;
	} else {
		if (externf) {
			printf("cannot define extern fuction\n");
			return -1;
		}
	}

	f_ret_type = type;

	emit_function_start(name, argc);
	label_count = 0; vardecl_ok = 1; loop_next = -1; loop_end = -1;
	if (parse_body()) return -1;
	emit_function_end();
	var_fini();
	return 0;
}

static int parse_fcall(char *name)
{
	int idx;
	int argc;
	int *argv;
	lex_consume();

	argc = func_get(name);
	if (argc == -1) {
		printf("undeclared function '%s'\n", name);
		return -1;
	}
	argv = malloc(argc * sizeof(int)); argc = 0;
	while (lex_peek() != ')') {
		if (argc) { if (lex_peek() != ',') return -1; lex_consume(); }
		idx = parse_expr(); if (idx == -1) return -1;
		argv[argc++] = idx;
		printf("parsed arg %d\n", argc);
	}
	lex_consume(); idx = func_get(name);
	if (idx != argc) {
		printf("function '%s' takes %d params, used with %d\n",
			name, idx, argc);
		return -1;
	}
	while (idx--) {
		emit_call_arg(argv[idx]); var_free(argv[idx]);
	}
	free(argv);
	idx = var_alloc(NULL, func_get_type(name));
	emit_call(name, argc, idx);
	return idx;
}

static int parse_if(void)
{
	int lab_start, lab_else, lab_end;
	int vdcl = vardecl_ok;

	vardecl_ok = 0;

	lex_consume();
	if (lex_peek() != '(') return -1; lex_consume();

	lab_start = parse_test_expr(); if (lab_start == -1) return -1;
	if (lex_peek() != ')') return -1; lex_consume();

	lab_else = label_count++; lab_end = lab_else;
	emit_jump(lab_else);

	emit_label(lab_start); 

	printf("[if] parsing THEN .. \n");
	if (parse_body()) return -1;
	printf("[if] THEN parsed\n");

	if (lex_peek() == LEX_ELSE) {
		lex_consume(); 
		lab_end = label_count++; emit_jump(lab_end);
		emit_label(lab_else);
		if (parse_body()) return -1;
	}
	emit_label(lab_end);

	vardecl_ok = vdcl;
	printf("[if] clean exit\n");
	return 0;
}

static int parse_while(void)
{
	int vdcl = vardecl_ok;
	int lab_loop, lab_body, lab_end;
	int my_next = loop_next; int my_end = loop_end;
	vardecl_ok = 0;

	lex_consume();
	if (lex_peek() != '(') return -1; lex_consume();

	lab_loop = label_count++; emit_label(lab_loop);
	lab_body = parse_test_expr(); if (lab_body == -1) return -1;
	if (lex_peek() != ')') return -1; lex_consume();

	lab_end = label_count++;
	emit_jump(lab_end);

	loop_next = lab_loop; loop_end = lab_end;

	emit_label(lab_body); 
	if (parse_body()) return -1;
	emit_jump(lab_loop); emit_label(lab_end);

	vardecl_ok = vdcl;
	loop_end = my_end; loop_next = my_next;
	return 0;
}

static int parse_for(void)
{
	int vdcl = vardecl_ok;
	int lab_loop, lab_body, lab_update, lab_end;
	int my_next = loop_next; int my_end = loop_end;

	vardecl_ok = 0;

	lex_consume();
	printf("EXPR0\n");
	if (lex_peek() != '(') return -1; lex_consume();

	printf("EXPR1\n");
	lex_peek();
	if (parse_assign()) return -1;
	printf("gothere!\n");
	if (lex_peek() != ';') return -1; lex_consume();

	lab_loop = label_count++; emit_label(lab_loop);

	printf("EXPR2\n"); lex_peek();
	lab_body = parse_test_expr(); if (lab_body == -1) return -1;
	if (lex_peek() != ';') return -1; lex_consume();

	lab_end = label_count++; emit_jump(lab_end);

	lab_update = label_count++; emit_label(lab_update);

	printf("EXPR3\n"); lex_peek();
	if (parse_assign()) return -1;
	if (lex_peek() != ')') return -1; lex_consume();

	emit_jump(lab_loop); emit_label(lab_body); 

	loop_next = lab_loop; loop_end = lab_end;

	if (parse_body()) return -1;
	emit_jump(lab_update); emit_label(lab_end);

	vardecl_ok = vdcl;
	loop_end = my_end; loop_next = my_next;
	return 0;
}

int parse_program(void)
{
	while (lex_peek() != LEX_NONE) {
		if ((lex_peek() != LEX_FUNC) &&
			(lex_peek() != LEX_EXTERN)) return -1;
		if (parse_function() == -1) return -1;
	}
	return 0; /* ? */
}

