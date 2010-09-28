#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "calc.h"

#define N_FUNCS 1024

typedef struct func {
	char *name;
	int argc;
	char type;
} func_t;

static func_t *functions;

void func_init(void)
{
	functions = malloc(N_FUNCS * sizeof(func_t));
	memset(functions, 0, N_FUNCS * sizeof(func_t));
}

void func_fini(void)
{
	free(functions); functions = NULL;
}

int func_decl(char *name, int argc, char type)
{
	int i;

	i = func_get(name);
	if (i != -1) {
		if (i == argc) return 0;
		printf("redeclaring function '%s'\n", name);
		return -1;
	}
	for (i = 0; i < N_FUNCS; i++)
		if (!(functions[i].name)) break;
	if (i == N_FUNCS) return -1;

	functions[i].name = name;
	functions[i].argc = argc;
	functions[i].type = type;
	printf("declared %s function '%s' with %d arguments\n",
		(functions[i].type == VAR_FLOAT) ? "float" : "int", name, argc);
	return 0;
}

int func_get(char *name)
{
	int i;
	for (i = 0; i < N_FUNCS; i++) {
		if (!functions[i].name) break;
		if (!strcmp(functions[i].name, name))
			return functions[i].argc;
	}
	return -1;
}


char func_get_type(char *name)
{
	int i;
	for (i = 0; i < N_FUNCS; i++) {
		if (!functions[i].name) break;
		if (!strcmp(functions[i].name, name))
			return functions[i].type;
	}
	return -1;
}



