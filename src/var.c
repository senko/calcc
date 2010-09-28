#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "calc.h"

typedef struct var {
	char used;
	char *name;
	long init;
	char type;
} var_t;

static var_t *variables;

void var_init(void)
{
	variables = malloc(N_VARS * sizeof(var_t));
	memset(variables, 0, N_VARS * sizeof(var_t));
}

void var_fini(void)
{
	free(variables); variables = NULL;
}

int var_alloc(char *name, char type)
{
	int i;

	if (name) {
		if (var_get(name) != -1) {
			printf("redeclaring variable '%s'\n", name);
			return -1;
		}
	}
	for (i = 0; i < N_VARS; i++) if (!variables[i].used) break;
	if (i == N_VARS) return -1;

	if (name)
		printf("declared variable '%s' (slot %d)\n", name, i);
	else
		printf("putting temporary in slot %d\n", i);

	variables[i].name = name;
	variables[i].type = type;
	variables[i].used = 1;
	return i;
}

void var_free(int idx)
{
	if (!variables[idx].name) variables[idx].used = 0;
}

char var_type(int idx)
{
	return variables[idx].type;
}

void var_set_type(int idx, char type)
{
	variables[idx].type = type;
}

int var_get(char *name)
{
	int i;
	printf("searching for '%s', total count %d\n",
		name, var_tcount());

	for (i = 0; i < N_VARS; i++) {
		if (!variables[i].name) continue;
		if (!strcmp(variables[i].name, name)) {
			printf("variable '%s' is in slot %d\n", name, i);
			return i;
		}
	}
	return -1;
}

int var_tcount(void)
{
	int i, cnt;
	for (i = 0, cnt = 0; i < N_VARS; i++) {
		if (variables[i].name) continue;
		if (variables[i].used) cnt++;
	}
	return cnt;
}


