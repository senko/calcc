#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char *input;
static char *ptr;

void input_init(char *txt)
{
	input = txt;
	ptr = input;
}

void input_finish()
{
	free(input);
}

char input_peek(void)
{
	return *ptr;
}

void input_consume(void)
{
	if (*ptr) ptr++;
}

