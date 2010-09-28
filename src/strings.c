#include <stdio.h>
#include <stdlib.h>

#include "calc.h"

#define N_STRINGS 1024

static char **strings;

void string_init(void)
{
	strings = malloc(N_STRINGS * sizeof(char *));
	memset(strings, 0, N_STRINGS * sizeof(char *));
}

void string_fini(void)
{
	free(strings); strings = NULL;
}

int string_decl(char *str)
{
	int i;
	for (i = 0; i < N_STRINGS; i++) if (!strings[i]) break;
	if (i == N_STRINGS) return -1;
	strings[i] = str;
	return i;
}

void string_output(void)
{
	int i;
	for (i = 0; i < N_STRINGS; i++)
		if (strings[i]) emit_asciiz(i, strings[i]);
}

