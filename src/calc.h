#ifndef _CALC_H_
#define _CALC_H_

#define N_VARS 1024

#define LEX_NONE 0
#define LEX_INTEGER 300
#define LEX_IDENTIFIER 301
#define LEX_STRING 302
#define LEX_FLOAT 303

#define LEX_IVAR 350
#define LEX_FUNC 351
#define LEX_RETURN 352
#define LEX_IF 353
#define LEX_ELSE 354
#define LEX_EXTERN 355
#define LEX_WHILE 356
#define LEX_FOR 357
#define LEX_CONTINUE 358
#define LEX_BREAK 359
#define LEX_FVAR 360

#define LEX_INPUT 400
#define LEX_PRINT 401

#define LEX_NE 500
#define LEX_EQ 501
#define LEX_GE 502
#define LEX_GT 503
#define LEX_LE 504
#define LEX_LT 505

#define VAR_NONE 0
#define VAR_INT 1
#define VAR_FLOAT 2

#define VAR_CONST 128

void input_init(char *txt);
void input_finish();
char input_peek(void);
void input_consume(void);

int emit_init(char *file);
void emit_finish(void);
void emit_variable(char *name);
void emit_function_start(char *name, int argc);
void emit_function_end(void);
void emit_call_arg(int idx);
void emit_call(char *name, int argc, int idx);
void emit_return(int retidx);
void emit_constant(int idx, long val);
void emit_mul(int left, int right, int result);
void emit_div(int left, int right, int result);
void emit_mod(int left, int right, int result);
void emit_add(int left, int right, int result);
void emit_sub(int left, int right, int result);
void emit_assign(int dest, int src);
void emit_label(int index);
void emit_test(int op, int left, int right, int label);
void emit_jump(int label);
void emit_extern(char *name);
void emit_asciiz(int index, char *string);
void emit_pushref(int index);
void emit_pushconst(int val);
void emit_cast_f2i(int dest, int src);
void emit_cast_i2f(int dest, int src);

void lex_init(void);
int lex_peek(void);
void lex_consume(void);
char *lex_get_string(void);
long lex_get_integer(void);

void var_init(void);
void var_fini(void);
int var_alloc(char *name, char type);
void var_free(int idx);
int var_get(char *name);
int var_tcount(void);
char var_type(int idx);
void var_set_type(int idx, char type);

void func_init(void);
void func_fini(void);
int func_decl(char *name, int argc, char type);
int func_get(char *name);
char func_get_type(char *name);

void string_init(void);
void string_fini(void);
int string_decl(char *str);
void string_output(void);

int parse_program(void);

#endif /* _CALC_H_ */

