#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "calc.h"

static char *read_file(char *fname)
{
	char *tmp;
	int len;
	FILE *in;

	in = fopen(fname, "r");
	if (!in) { perror("can't open input"); return NULL; }
	fseek(in, 0, SEEK_END); len = ftell(in); rewind(in);
	tmp = malloc(len + 1);
	fread(tmp, len, 1, in); tmp[len] = 0;
	fclose(in);
	return tmp;
}

int main(int argc, char *argv[])
{
	char *tmp;
	if (argc != 3) {
		printf("Usage: %s <input> <output>\n", argv[0]);
		return EXIT_FAILURE;
	}

	tmp = read_file(argv[1]); if (!tmp) return EXIT_FAILURE;
	input_init(tmp); func_init(); string_init(); lex_init();
	if (emit_init(argv[2])) return EXIT_FAILURE;
	if (parse_program()) {
		emit_finish();
		unlink(argv[2]);
		return EXIT_FAILURE;
	}
	string_output();
	printf("compile succeeded\n");
	emit_finish();
	return EXIT_SUCCESS;
}

