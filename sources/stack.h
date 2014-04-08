#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _stack{
	int top;
	int value[1024];
}stack;


void stack_init(stack* pstk);
int stack_push(stack* pstk, int value);
int stack_pop(stack* pstk);
int stack_is_empty(stack* pstk);
int stack_get_top(stack* pstk);

