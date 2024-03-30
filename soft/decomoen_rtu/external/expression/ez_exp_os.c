#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "ez_exp_os.h"

void *ez_exp_malloc(size_t size)
{
    return malloc(size);
}

void ez_exp_free(void *p)
{
    if (p) free(p);
}

void ez_exp_putc(const char ch)
{
    putchar(ch);
}

