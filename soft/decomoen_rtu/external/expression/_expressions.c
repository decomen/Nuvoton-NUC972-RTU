
static double _expression_ternary(struct ez_exp *e, ez_flag *flag);
static double _expression_comma(struct ez_exp *e, ez_flag *flag);
static double _expression_max(struct ez_exp *e, ez_flag *flag);
static double _expression_min(struct ez_exp *e, ez_flag *flag);

/**
赋值操作
*/
static double _expression_set(int n, struct ez_exp *e, ez_flag *flag)
{
    double f1 = e->R[n], f2;
    double f3 = f1;
    ez_token t = e->sym;
    _accept(e, e->sym);
    if (t == T_SET_DPLUS) {
        f1++;
    } else if (t == T_SET_DMINUS) {
        f1--;
    } else {
        f2 = _expression(e, flag);
        //printf("n,f1,f2:%d,%lf,%lf\n", n, f1, f2);
        switch (t) {
        case T_SET_SHIFT_LEFT_NUM:      f1 = (_ul(f1) << _ul(f2)); break;
        case T_SET_SHIFT_RIGHT_NUM:     f1 = (_ul(f1) >> _ul(f2)); break;
        case T_SET_MOD_NUM:             f1 = (_ul(f1) % _ul(f2)); break;
        case T_SET_OR_NUM:              f1 = (_ul(f1) | _ul(f2)); break;
        case T_SET_AND_NUM:             f1 = (_ul(f1) & _ul(f2)); break;
        case T_SET_XOR_NUM:             f1 = (_ul(f1) ^ _ul(f2)); break;
        case T_SET_PLUS_NUM:            f1 += f2; break;
        case T_SET_MINUS_NUM:           f1 -= f2; break;
        case T_SET_MULTIPLY_NUM:        f1 *= f2; break;
        case T_SET_DIVIDE_NUM:          f1 /= f2; break;
        case T_SET_EQUAL:               f3 = f1 = f2; break;
        default: ;
        }
    }
    if (!EZ_SKIP(flag)) e->R[n] = f1;
    return f3;
}

static void _expression_skip_until_right_banana(struct ez_exp *e)
{
    int left = 1, right = 0;
    if (e->sym == T_LEFT_BANANA) left++;
    while (left != right) {
        _accept(e, e->sym);
        if (e->sym == T_LEFT_BANANA) {
            left++;
        } else if (e->sym == T_RIGHT_BANANA) {
            right++;
        } else if (e->sym == T_ERROR || e->sym == T_EOF) {
            return;
        }
    }
}

static void _expression_skip_until_right_braces(struct ez_exp *e)
{
    int left = 1, right = 0;
    if (e->sym == T_LEFT_BRACES) left++;
    while (left != right) {
        _accept(e, e->sym);
        if (e->sym == T_LEFT_BRACES) {
            left++;
        } else if (e->sym == T_RIGHT_BRACES) {
            right++;
        } else if (e->sym == T_ERROR || e->sym == T_EOF) {
            return;
        }
    }
}

/**
三元运算 
ifelse(a,{b},{c}) 
如果 a!=0 执行返回b, 跳过c
如果 a==0 执行返回c, 跳过b
*/

static void _expression_check_ternary(struct ez_exp *e, ez_flag *flag)
{
    _expression(e, flag);
    _expect(e, T_DOT, "ifelse(a,{b},{c}) a, lost ','");
    _expect(e, T_LEFT_BRACES, "ifelse(a,{b},{c}) {b} lost '{'");
    _expression_comma(e, flag);
    _expect(e, T_RIGHT_BRACES, "ifelse(a,{b},{c}) {b} lost '}'");
    _expect(e, T_DOT, "ifelse(a,{b},{c}) {b},{c} lost ','");
    _expect(e, T_LEFT_BRACES, "ifelse(a,{b},{c}) {c} lost '{'");
    _expression_comma(e, flag);
    _expect(e, T_RIGHT_BRACES, "ifelse(a,{b},{c}) {c} lost '}'");
}

static double _expression_ternary(struct ez_exp *e, ez_flag *flag)
{
    double f1;
    f1 = _expression(e, flag);
    _expect(e, T_DOT, "ifelse(a,{b},{c}) a, lost ','");
    if (EZ_CHECK(e)) {
        _expression_check_ternary(e, flag);
    } else if (EZ_SKIP(flag) || EZ_RETURN(e)) {
        _expression_skip_until_right_banana(e);
    } else if (f1 != 0) {
        _expect(e, T_LEFT_BRACES, "ifelse(a,{b},{c}) {b} lost '{'");
        f1 = _expression_comma(e, flag);
        _expect(e, T_RIGHT_BRACES, "ifelse(a,{b},{c}) {b} lost '}'");
        _expression_skip_until_right_banana(e);
    } else {
        _expect(e, T_LEFT_BRACES, "ifelse(a,{b},{c}) {b} lost '{'");
        _expression_skip_until_right_braces(e);
        _expect(e, T_RIGHT_BRACES, "ifelse(a,{b},{c}) {b} lost '}'");
        _expect(e, T_DOT, "ifelse(a,{b},{c}) {b},{c} lost ','");
        _expect(e, T_LEFT_BRACES, "ifelse(a,{b},{c}) {c} lost '{'");
        f1 = _expression_comma(e, flag);
        _expect(e, T_RIGHT_BRACES, "ifelse(a,{b},{c}) {c} lost '}'");
    }
    return f1;
}

/**
if 逻辑 
if(a) {} else if(b) {} else {}
*/

// just skip {}
static void _if_skip_if(struct ez_exp *e)
{
    _expect(e, T_LEFT_BRACES, "if(a) {} lost '{'");
    _expression_skip_until_right_braces(e);
    _expect(e, T_RIGHT_BRACES, "if(a) {} lost '}'");
}

// skip () and {}
static void _if_skip_else_if(struct ez_exp *e)
{
    _expect(e, T_LEFT_BRACES, "else if(b) {} lost '{'");
    _expression_skip_until_right_braces(e);
    _expect(e, T_RIGHT_BRACES, "else if(b) {} lost '}'");
}

// just skip {}
static void _if_skip_else(struct ez_exp *e)
{
    _expect(e, T_LEFT_BRACES, "else {} lost '{'");
    _expression_skip_until_right_braces(e);
    _expect(e, T_RIGHT_BRACES, "else {} lost '}'");
}

static void _if_skip_else_remain(struct ez_exp *e)
{
    while (e->sym == T_IF_FUNC_ELSE) {
        _accept(e, e->sym);
        if (e->sym == T_IF_FUNC_IF) {
            _accept(e, e->sym);
            _expect(e, T_LEFT_BANANA, "else if(b) lost '('");
            _expression_skip_until_right_banana(e);
            _expect(e, T_RIGHT_BANANA, "else if(b) lost ')'");
            _if_skip_else_if(e);
        } else {
            _if_skip_else(e);
            break;
        }
    }
}

static double _if_skip_all(struct ez_exp *e)
{
    _expect(e, T_LEFT_BANANA, "if(a) lost '('");
    _expression_skip_until_right_banana(e);
    _expect(e, T_RIGHT_BANANA, "if(a) lost ')'");
    _if_skip_if(e);
    _if_skip_else_remain(e);
    return 0;
}

static double _if_check(struct ez_exp *e, ez_flag *flag)
{
    char c_if = false; EZ_EXP_UNUSED(c_if);
    double f1 = 0;
    _expect(e, T_LEFT_BANANA, "if(a) lost '('");
    c_if = _expression(e, flag);
    _expect(e, T_RIGHT_BANANA, "if(a) lost ')'");
    _expect(e, T_LEFT_BRACES, "if(a) {} lost '{'");
    _expression_comma(e, flag);
    _expect(e, T_RIGHT_BRACES, "if(a) {} lost '}'");
    while (e->sym == T_IF_FUNC_ELSE) {
        _accept(e, e->sym);
        if (e->sym == T_IF_FUNC_IF) {
            _accept(e, e->sym);
            _expect(e, T_LEFT_BANANA, "else if(b) lost '('");
            c_if = _expression(e, flag);
            _expect(e, T_RIGHT_BANANA, "else if(b) lost ')'");
            _expect(e, T_LEFT_BRACES, "else if(b) {} lost '{'");
            _expression_comma(e, flag);
            _expect(e, T_RIGHT_BRACES, "else if(b) {} lost '}'");
        } else {
            _expect(e, T_LEFT_BRACES, "else {} lost '{'");
            _expression_comma(e, flag);
            _expect(e, T_RIGHT_BRACES, "else {} lost '}'");
            break;
        }
    }
    return f1;
}

static double _if_do(struct ez_exp *e, ez_flag *flag)
{
    char c_if = false;
    double f1 = 0;
    _expect(e, T_LEFT_BANANA, "if(a) lost '('");
    c_if = (_expression(e, flag) != 0);
    _expect(e, T_RIGHT_BANANA, "if(a) lost ')'");
    if (c_if) {
        _expect(e, T_LEFT_BRACES, "if(a) {} lost '{'");
        f1 = _expression_comma(e, flag);
        _expect(e, T_RIGHT_BRACES, "if(a) {} lost '}'");
        _if_skip_else_remain(e);
        return f1;
    }
    _if_skip_if(e);
    while (e->sym == T_IF_FUNC_ELSE) {
        _accept(e, e->sym);
        if (e->sym == T_IF_FUNC_IF) {
            _accept(e, e->sym);
            _expect(e, T_LEFT_BANANA, "else if(b) lost '('");
            c_if = (_expression(e, flag) != 0);
            _expect(e, T_RIGHT_BANANA, "else if(b) lost ')'");
            if (c_if) {
                _expect(e, T_LEFT_BRACES, "else if(b) {} lost '{'");
                f1 = _expression_comma(e, flag);
                _expect(e, T_RIGHT_BRACES, "else if(b) {} lost '}'");
                _if_skip_else_remain(e);
                return f1;
            }
            _if_skip_else_if(e);
        } else {
            _expect(e, T_LEFT_BRACES, "else {} lost '{'");
            f1 = _expression_comma(e, flag);
            _expect(e, T_RIGHT_BRACES, "else {} lost '}'");
            return f1;
        }
    }
    return f1;
}

static double _expression_if(struct ez_exp *e, ez_flag *flag)
{
    if (EZ_CHECK(e)) {
        return _if_check(e, flag);
    } else if (EZ_SKIP(flag) || EZ_RETURN(e)) {
        return _if_skip_all(e);
    } else {
        return _if_do(e, flag);
    }
}

/**
for 循环
for(init;cond;op) {
    // do something
}
*/

static char __for_get_cond(struct ez_exp *e, char *p, unsigned short ln_num, ez_flag *flag)
{
    bool _cond = false;
    e->tokenizer.p = p; e->tokenizer.ln_num = ln_num;
    _accept(e, e->sym);
    _expect(e, T_SEMICOLON, "for(;;) lost ';'");
    if (e->sym != T_SEMICOLON) {
        _cond = _expression(e, flag);
    } else {
        _cond = true;
    }

    return _cond && !EZ_SKIP(flag);
}

static void __for_do_op(struct ez_exp *e, char *p, unsigned short ln_num, ez_flag *flag)
{
    e->tokenizer.p = p; e->tokenizer.ln_num = ln_num;
    _accept(e, e->sym);
    _expect(e, T_SEMICOLON, "for(;;) lost ';'");
    if (e->sym != T_RIGHT_BANANA) {
        _expression(e, flag);
        while (e->sym == T_DOT) {
            _accept(e, e->sym);
            _expression(e, flag);
        }
    }
}

static void __for_do_comma(struct ez_exp *e, char *p, unsigned short ln_num, ez_flag *flag)
{
    e->tokenizer.p = p; e->tokenizer.ln_num = ln_num;
    _accept(e, e->sym);
    _expect(e, T_LEFT_BRACES, "for(;;) {} lost '{'");
    _expression_comma(e, flag);
}

static void __for_skip_comma(struct ez_exp *e, char *p, unsigned short ln_num)
{
    e->tokenizer.p = p; e->tokenizer.ln_num = ln_num;
    _accept(e, e->sym);
    _expect(e, T_LEFT_BRACES, "for(;;) {} lost '{'");
    _expression_skip_until_right_braces(e);
    _expect(e, T_RIGHT_BRACES, "for(;;) {} lost '}'");
}

static double _expression_for_check(struct ez_exp *e)
{
    char *p_cond, *p_op, *p_comma;
    unsigned short ln_num_cond, ln_num_op, ln_num_comma;
    char _cond = false; EZ_EXP_UNUSED(_cond);
    ez_flag flag = {false, false};
    // init: 可以用逗号隔开多条语句
    _expect(e, T_LEFT_BANANA, "for(;;) lost '('");
    if (e->sym != T_SEMICOLON) {
        _expression(e, &flag);
        while (e->sym == T_DOT) {
            _accept(e, e->sym);
            _expression(e, &flag);
        }
    }
    // init: 可以用逗号隔开多条语句
    p_cond = e->tokenizer.p - 1; ln_num_cond = e->tokenizer.ln_num;
    _expect(e, T_SEMICOLON, "for(;;) lost ';'");
    _cond = __for_get_cond(e, p_cond, ln_num_cond, &flag);
    p_op = e->tokenizer.p - 1; ln_num_op = e->tokenizer.ln_num;
    _expect(e, T_SEMICOLON, "for(;;) lost ';'");
    __for_do_op(e, p_op, ln_num_op, &flag);
    _expect(e, T_RIGHT_BANANA, "for(;;) lost ')'");
    p_comma = e->tokenizer.p - 1; ln_num_comma = e->tokenizer.ln_num;
    _expect(e, T_LEFT_BRACES, "for(;;) {} lost '{'");
    __for_do_comma(e, p_comma, ln_num_comma, &flag);
    _expect(e, T_RIGHT_BRACES, "for(;;) {} lost '}'");
    return 0;
}

static double _expression_skip_for_all(struct ez_exp *e)
{
    _expect(e, T_LEFT_BANANA, "for(;;) lost '('");
    _expression_skip_until_right_banana(e);
    _expect(e, T_RIGHT_BANANA, "for(;;) lost ')'");
    _expect(e, T_LEFT_BRACES, "for(;;) {} lost '{'");
    _expression_skip_until_right_braces(e);
    _expect(e, T_RIGHT_BRACES, "for(;;) {} lost '}'");
    return 0;
}

static double _expression_for_do(struct ez_exp *e)
{
    char *p_cond, *p_op, *p_comma;
    unsigned short ln_num_cond, ln_num_op, ln_num_comma;
    char _cond = false;
    ez_flag flag = {false, false};
    // init: 可以用逗号隔开多条语句
    _expect(e, T_LEFT_BANANA, "for(;;) lost '('");
    if (e->sym != T_SEMICOLON) {
        _expression(e, &flag);
        while (e->sym == T_DOT) {
            _accept(e, e->sym);
            _expression(e, &flag);
        }
    }
    // init: 可以用逗号隔开多条语句
    p_cond = e->tokenizer.p - 1; ln_num_cond = e->tokenizer.ln_num;
    _expect(e, T_SEMICOLON, "for(;;) lost ';'");
    _cond = __for_get_cond(e, p_cond, ln_num_cond, &flag);
    
    p_op = e->tokenizer.p - 1; ln_num_op = e->tokenizer.ln_num;
    _expect(e, T_SEMICOLON, "for(;;) lost ';'");
    _expression_skip_until_right_banana(e);
    _expect(e, T_RIGHT_BANANA, "for(;;) lost ')'");
    
    p_comma = e->tokenizer.p - 1; ln_num_comma = e->tokenizer.ln_num;
    _expect(e, T_LEFT_BRACES, "for(;;) {} lost '{'");
    if (_cond) {
        __for_do_comma(e, p_comma, ln_num_comma, &flag);
    } else {
        _expression_skip_until_right_braces(e);
    }
    _expect(e, T_RIGHT_BRACES, "for(;;) {} lost '}'");

    if (!flag.is_break && !e->is_return && !e->is_error && _cond) {
        __for_do_op(e, p_op, ln_num_op, &flag);
        while (!flag.is_break && !e->is_return && !e->is_error && _cond) {
            _cond = __for_get_cond(e, p_cond, ln_num_cond, &flag);
            if (_cond) {
                __for_do_comma(e, p_comma, ln_num_comma, &flag);
                flag.is_continue = false;
                if (!flag.is_break) {
                    __for_do_op(e, p_op, ln_num_op, &flag);
                }
            }
        }
    }
    __for_skip_comma(e, p_comma, ln_num_comma);
    return 0;
}

static double _expression_for(struct ez_exp *e, ez_flag *flag)
{
    if (EZ_CHECK(e)) {
        return _expression_for_check(e);
    } else if (EZ_SKIP(flag) || EZ_RETURN(e)) {
        return _expression_skip_for_all(e);
    } else {
        return _expression_for_do(e);
    }
}

/**
多表达式,如果无return返回最右值
{a;b;c;d} 返回d值
*/
static double _expression_comma(struct ez_exp *e, ez_flag *flag)
{
    double f1;
    f1 = _expression(e, flag);
    while (e->sym == T_SEMICOLON || e->last_sym == T_RIGHT_BRACES) {
        if (e->sym == T_SEMICOLON) {
            _accept(e, e->sym);
        }
        if (e->sym == T_SEMICOLON) {
            // 跳过空 ;
            continue;
        } else if (e->sym == T_RIGHT_BRACES || e->sym == T_EOF) {
            break;
        }
        f1 = _expression(e, flag);
    }
    return f1;
}

/**
最大值
max(a,b,c,d,...)
*/
static double _expression_max(struct ez_exp *e, ez_flag *flag)
{
    double _max;
    _max = _expression(e, flag);
    while (e->sym == T_DOT) {
        _accept(e, e->sym);
        double f1 = _expression(e, flag);
        if (f1 > _max) _max = f1;
    }
    return _max;
}

/**
最小值
min(a,b,c,d,...)
*/
static double _expression_min(struct ez_exp *e, ez_flag *flag)
{
    double _min;
    _min = _expression(e, flag);
    while (e->sym == T_DOT) {
        _accept(e, e->sym);
        double f1 = _expression(e, flag);
        if (f1 < _min) _min = f1;
    }
    return _min;
}

/**
打印
print(a,b,c,d,...)
*/

static void _expression_print_value(struct ez_exp *e, ez_flag *flag)
{
    if (e->sym == T_STRING) {
        int i;
        for(i = 0; i < e->tokenizer.actual_size; i++) {
            char c0 = e->tokenizer.actual_string[i], c1 = e->tokenizer.actual_string[i + 1];
            if (c0 == '\\') {
                switch(c1) {
                case 'n': ez_exp_putc('\n'); i++; break;
                case 'r': ez_exp_putc('\r'); i++; break;
                case 'b': ez_exp_putc('\b'); i++; break;
                case '\\': ez_exp_putc('\\'); i++; break;
                case '\"': ez_exp_putc('\"'); i++; break;
                case '\'': ez_exp_putc('\''); i++; break;
                }
            } else {
                ez_exp_putc(e->tokenizer.actual_string[i]);
            }
        }
        _accept(e, e->sym);
    } else {
        _print_number(_expression(e, flag)); 
    }
}

static double _expression_print(struct ez_exp *e, ez_flag *flag)
{
    int n = 1;
    _expression_print_value(e, flag);
    while (e->sym == T_DOT) {
        _accept(e, e->sym);
        _expression_print_value(e, flag);
        n++;
    }
    return (double)n;
}

