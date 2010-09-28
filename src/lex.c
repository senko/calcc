#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "calc.h"

#define MAX_TOK 1024
#define MAX_STR 16384

static int lex_token_type;
static char *lex_token_string;
static long lex_token_integer;

struct keyword {
	char *name;
	int type;
};

static struct keyword keywords[] = {
	{ "int", LEX_IVAR },
	{ "float", LEX_FVAR },
	{ "function", LEX_FUNC },
	{ "return", LEX_RETURN },
	{ "if", LEX_IF },
	{ "else", LEX_ELSE },
	{ "extern", LEX_EXTERN },
	{ "while", LEX_WHILE },
	{ "break", LEX_BREAK },
	{ "continue", LEX_CONTINUE },
	{ "for", LEX_FOR },
	{ "input", LEX_INPUT },
	{ "print", LEX_PRINT },
	{ NULL, LEX_NONE }
};

static char *detect_identifier(void)
{
	char tmp[MAX_TOK+1];
	int i;

	for (i = 0; i < MAX_TOK; i++) {
		tmp[i] = input_peek();
		if ((tmp[i] != '_') && !isalnum(tmp[i])) break;
		input_consume();
	}
	tmp[i] = 0;

	if (!i) return NULL;
	return strdup(tmp);
}

static char *detect_string(void)
{
	char tmp[MAX_STR + 1];
	int i;
	int c;

	input_consume();
	for (i = 0; (i < MAX_STR) && input_peek(); i++) {
		c = input_peek(); if (c == '"') break;
		if (c == '\\') {
			input_consume(); c = input_peek();
			switch (c) {
				case 't': c = '\t'; break;
				case 'n': c = '\n'; break;
				case 'r': c = '\r'; break;
				case 'a': c = '\a'; break;
			}
		}
		tmp[i] = c;
		input_consume();
	}
	if (input_peek() == '"') input_consume();

	tmp[i] = 0;
	return strdup(tmp);
}

static int detect_float(int sgn, long intpart)
{
	char c;
	char tmp[MAX_TOK]; int i;
	float *f;

	input_consume();
	sprintf(tmp, "%s%ld.", (sgn == -1) ? "-" : "", intpart);
	i = strlen(tmp);

	lex_token_type = LEX_NONE;

	while (isdigit(c = input_peek())) { tmp[i++] = c; input_consume(); }
	if (toupper(input_peek()) == 'E') {
		input_consume(); tmp[i++] = 'E';
		c = input_peek();
		if ((c != '+') && (c != '-')) {
			lex_token_type = LEX_NONE; return LEX_NONE;
		}
		tmp[i++] = c;
		while (isdigit(c = input_peek())) {
			tmp[i++] = c;
			input_consume();
		}
	}
	tmp[i] = 0;
	f = (float *) &lex_token_integer;
	*f = (float) atof(tmp);

	printf("LEX_float: %f\n", *f);
	lex_token_type = LEX_FLOAT;
	return LEX_FLOAT;
}

static int detect_integer(void)
{
	char c;
	int sgn = 1;
	long x = 0;

	if (input_peek() == '-') { sgn = -1; input_consume(); }
	if (!isdigit(input_peek())) {
		lex_token_type = '-';
		return '-';
	}

	while (isdigit(c = input_peek())) {
		x = 10 * x + ((long) (c - '0'));
		input_consume();
	}
	if (input_peek() == '.') return detect_float(sgn, x);

	lex_token_type = LEX_INTEGER;
	lex_token_integer = x * sgn;
	return LEX_INTEGER;
}

static int detect_token(void)
{
	char x;
	char *tmp;
	int i;

	while (isspace(input_peek())) input_consume();

	x = input_peek();
	printf("Detect token ... x = [%c]\n", x);
	if (isalpha(x)) {
		tmp = detect_identifier();
		printf("detected identifier: [%s]\n", tmp);
		if (tmp) {
			for (i = 0; keywords[i].name; i++) {
				if (strcmp(keywords[i].name, tmp)) continue;
				lex_token_type = keywords[i].type;
				free(tmp);
				return lex_token_type;
			}
			lex_token_type = LEX_IDENTIFIER;
			lex_token_string = tmp;
			return LEX_IDENTIFIER;
		}
		printf("lexical error\n");
		return -1;
	}
	if (x == '"') {
		tmp = detect_string();
		lex_token_type = LEX_STRING;
		lex_token_string = tmp;
		printf("detected string token [%s]\n", tmp);
		return LEX_STRING;
	}
	if (isdigit(x) || (x == '-')) return detect_integer();

	lex_token_type = x;
	input_consume();

	/* multi-char operators */
	switch (lex_token_type) {
		case '!': if (input_peek() != '=') break;
			input_consume(); lex_token_type = LEX_NE; break;
		case '>':
			if (input_peek() == '=') {
				input_consume(); lex_token_type = LEX_GE;
			} else {
				lex_token_type = LEX_GT;
			}
			break;
		case '<':
			if (input_peek() == '=') {
				input_consume(); lex_token_type = LEX_LE;
			} else {
				lex_token_type = LEX_LT;
			}
			break;
		case '=':
			if (input_peek() == '=') {
				input_consume(); lex_token_type = LEX_EQ;
			}
			break;	
	}
	return lex_token_type;
}

void lex_init(void)
{
	lex_token_type = LEX_NONE;
	lex_token_string = NULL;
	lex_token_integer = 0;
}

int lex_peek(void)
{
	if (lex_token_type == LEX_NONE) detect_token();
	return lex_token_type;
}

void lex_consume(void)
{
	if (lex_token_type == LEX_NONE) lex_peek();
	if (lex_token_string) free(lex_token_string);

	printf("consuming(%d)\n", lex_token_type);
	lex_token_type = LEX_NONE;
	lex_token_string = NULL;
	lex_token_integer = 0;
}

char *lex_get_string(void)
{
	return lex_token_string;
}

long lex_get_integer(void)
{
	return lex_token_integer;
}

