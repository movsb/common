#include "stack.h"

void stack_init(stack* pstk)
{
	pstk->top=-1;
}

int stack_push(stack* pstk, int value)
{
	pstk->value[++pstk->top] = value;
	return 0;
}

int stack_pop(stack* pstk)
{
	return pstk->value[pstk->top--];
}

int stack_is_empty(stack* pstk)
{
	return pstk->top == -1;
}

int stack_get_top(stack* pstk)
{
	return pstk->value[pstk->top];
}
