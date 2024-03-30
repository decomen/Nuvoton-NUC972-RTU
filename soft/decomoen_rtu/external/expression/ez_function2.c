
#include "ez_function2.h"
#include "board.h"

#define _ul(n)     (unsigned long)n

static double _doset(double number1, double number2)
{
    int index = (int)(number1 + 0.5) - 1;
    int state = (int)(number2 + 0.5);
    g_xDOResultReg.xDOResult.usDO_xx[index] = (state!=0?PIN_SET:PIN_RESET);
    //printf("_doset : %d,%d\n", index, state);
    if (index < RELAYS_OUTPUT_NUM) {
        return index >= 0 ? das_do_set_do_state(DAS_DIDO_TYPE_RELAY, index, state) : -1;
    } else {
        //printf("ez: das_do_set_do_state\r\n");
        return index >= 0 ? das_do_set_do_state(DAS_DIDO_TYPE_TTL, index - RELAYS_OUTPUT_NUM, state) : -1;
    }
}

static const function_2 token_to_functions_2[T_FUNC_MAX_TOKEN] = {
    [T_FUNC_DO_SET]     = _doset,
};

bool is_function2_token(ez_token t)
{
    return (t < T_FUNC_MAX_TOKEN && token_to_functions_2[t] != NULL);
}

function_2 get_function2(ez_token t)
{
    return (t < T_FUNC_MAX_TOKEN ? token_to_functions_2[t] : NULL);
}

