#ifndef __EZ_EXP_OS_H__
#define __EZ_EXP_OS_H__

#include <stdio.h>

void *ez_exp_malloc(size_t size);
void ez_exp_free(void *p);
void ez_exp_putc(const char ch);

#endif

