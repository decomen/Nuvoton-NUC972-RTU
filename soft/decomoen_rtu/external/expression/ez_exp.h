#ifndef __EZ_EXP_H__
#define __EZ_EXP_H__

#define EZ_EXP_UNUSED(x)   (void)x

void *ez_exp_create(void);
int ez_exp_check(void *ez_exp, char *expstring, const char **errmsg);
double ez_exp_run(void *ez_exp, char *expstring, void *param, const char **errmsg);
void ez_exp_destroy(void *ez_exp);

double evaluate(char *exp, char *name, int *is_error);

#endif // __EXP_PARSER_H__
