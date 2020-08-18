#include <stdio.h>
#include <assert.h> /*assert*/
#include <stdlib.h> /*malloc*/

#include "calculator.h" /*API*/
#include "stack.h" /*StackCreate*/

#define NUM_CHARS (256)
#define SIZE_STACK_NUM (100)
#define SIZE_STACK_OP (100)

enum STATES_ENUM {WAIT_NUM, WAIT_OP, ERROR, COMPUTE, NUM_STATES};
enum ASSOCIATIVITY_ENUM {LR, RL};
enum PRECENDENCIES_ENUM {PRE_ADD = 1, PRE_SUB = 1, PRE_MUL = 2, PRE_DIV = 2, PRE_OPEN = 0, PRE_CLOSE = 3};

typedef status_t (*handle_func_t)(parser_t *parser);
typedef status_t (*op_func_t)(double num1, double num2, double *result);
typedef enum STATES_ENUM state_name_t;

struct state
{
    state_name_t next_state; 
    handle_func_t handle_func; 
};

struct op
{
    int precendence;
    int associativity;
    op_func_t operation_func; 
};

struct calc
{
    stack_t *stack_operators;
    stack_t *stack_numbers;
};

struct parser
{
    calc_t *calc;
    state_name_t current_state;
    char *str;
};

state_t FSM[NUM_STATES][NUM_CHARS] = {0};
op_t OPERATORS[NUM_CHARS] = {0};

void InitAll();
void InitOperators();
void InitFSM();

status_t Calculate (char *str, double *result);
parser_t *ParserCreate (char *str);
void ParserDestroy(parser_t *parser);
status_t ParserOp(parser_t *parser);
status_t ParserNum (parser_t *parser);
status_t ParserOpen(parser_t *parser);
status_t ParserClose(parser_t *parser);
status_t ParserError(parser_t *parser);
status_t ParserDoNothing (parser_t *parser);
status_t ParserEOS(parser_t *parser, void *result);

calc_t *CalcCreate();
void CalcDestroy(calc_t *calc);
status_t CalcPushOP(calc_t *calc, void *op);
status_t CalcPushNum(calc_t *calc, void *num);
status_t CalcPushOpenParan(calc_t *calc, void *open);
status_t CalcPushCloseParan(calc_t *calc, void *close);
status_t CalcGetResult(calc_t *calc, void *result);

static status_t Add (double num1, double num2, double *result);
static status_t Subtract (double num1, double num2, double *result);
static status_t Multiply (double num1, double num2, double *result);
static status_t Divide (double num1, double num2, double *result);
static status_t DummyFunc (double num1, double num2, double *result);
static status_t Fold(calc_t *calc);
/* static status_t Power (double num1, double num2, double *result);*/

status_t Calculate (char *str, double *result)
{
   parser_t *parser = NULL;
   status_t status = VALID;
   state_name_t next_state = WAIT_NUM;
   
   assert(NULL != str);
   assert(NULL != result);

   parser = ParserCreate(str);
   if(NULL == parser)
   {
       return MEMORY_ALLOCATION_FAILED;
   }

   while(COMPUTE != next_state && ERROR != next_state)
   {
       next_state = FSM[parser->current_state][(int)(*(parser->str))].next_state;
       status = FSM[parser->current_state][(int)(*(parser->str))].handle_func(parser);
       if(VALID != status)
       {
           ParserDestroy(parser);
           
           return status;
       }
       parser->current_state = next_state;
       
   }

    if (next_state == COMPUTE && status == VALID)
    {
        status = ParserEOS(parser, result);
    }

   ParserDestroy(parser);

   return status;;
}

void InitAll()
{
    InitOperators();
    InitFSM();
} 

void InitOperators()
{
    OPERATORS['+'].precendence = PRE_ADD;
    OPERATORS['+'].associativity = LR;
    OPERATORS['+'].operation_func = Add;

    OPERATORS['-'].precendence = PRE_SUB;
    OPERATORS['-'].associativity = LR;
    OPERATORS['-'].operation_func = Subtract;

    OPERATORS['*'].precendence = PRE_MUL;
    OPERATORS['*'].associativity = LR;
    OPERATORS['*'].operation_func = Multiply;

    OPERATORS['/'].precendence = PRE_DIV;
    OPERATORS['/'].associativity = LR;
    OPERATORS['/'].operation_func = Divide;

    OPERATORS[')'].precendence = PRE_CLOSE;
    OPERATORS[')'].associativity = LR;
    OPERATORS[')'].operation_func = DummyFunc;
   
    OPERATORS['('].precendence = PRE_OPEN;
    OPERATORS['('].associativity = LR;
    OPERATORS['('].operation_func = DummyFunc;
    
    /*OPERATORS['^'].precendence = 4;
    OPERATORS['^'].associativity = RL;
    OPERATORS['^'].operation_func = Power;*/
}

void InitFSM()
{
    int i = 0;

    for(i = 0; i <= NUM_CHARS; ++i)
    {
        FSM[WAIT_NUM][i].next_state = ERROR;
        FSM[WAIT_NUM][i].handle_func = ParserError;
        FSM[WAIT_OP][i].next_state = ERROR;
        FSM[WAIT_OP][i].handle_func = ParserError;
        FSM[ERROR][i].next_state = ERROR;
        FSM[ERROR][i].handle_func = ParserError;
    }

    for(i = 0; i < 10; ++i)
    {
        FSM[WAIT_NUM][i + '0'].next_state = WAIT_OP;
        FSM[WAIT_NUM][i + '0'].handle_func = ParserNum;
    }

    FSM[WAIT_NUM]['+'].next_state = WAIT_OP;
    FSM[WAIT_NUM]['+'].handle_func = ParserNum;

    FSM[WAIT_NUM]['-'].next_state = WAIT_OP;
    FSM[WAIT_NUM]['-'].handle_func = ParserNum;

    FSM[WAIT_NUM]['.'].next_state = WAIT_OP;
    FSM[WAIT_NUM]['.'].handle_func = ParserNum;

    FSM[WAIT_NUM]['('].next_state = WAIT_NUM;
    FSM[WAIT_NUM]['('].handle_func = ParserOpen;

    FSM[WAIT_OP]['+'].next_state = WAIT_NUM;
    FSM[WAIT_OP]['+'].handle_func = ParserOp;

    FSM[WAIT_OP]['-'].next_state = WAIT_NUM;
    FSM[WAIT_OP]['-'].handle_func = ParserOp;

    FSM[WAIT_OP]['*'].next_state = WAIT_NUM;
    FSM[WAIT_OP]['*'].handle_func = ParserOp;

    FSM[WAIT_OP]['/'].next_state = WAIT_NUM;
    FSM[WAIT_OP]['/'].handle_func = ParserOp;

    FSM[WAIT_OP][')'].next_state = WAIT_OP;
    FSM[WAIT_OP][')'].handle_func = ParserClose;

    FSM[WAIT_OP][0].next_state = COMPUTE;
    FSM[WAIT_OP][0].handle_func = ParserDoNothing;
}

calc_t *CalcCreate()
{
   stack_t *stack_op = NULL;
   stack_t *stack_num = NULL;
   calc_t *calc = (calc_t *)malloc(sizeof(calc_t));

   if(NULL == calc)
   {
       return NULL;
   }

   stack_op = StackCreate(SIZE_STACK_OP, sizeof(char));
   if (NULL == stack_op)
   {
       free(calc);
       return NULL;
   }

   stack_num = StackCreate(SIZE_STACK_NUM, sizeof(double));
   if (NULL == stack_num)
   {
       free(calc);
       StackDestroy(stack_op);
       return NULL;
   }

   calc->stack_numbers = stack_num;
   calc->stack_operators = stack_op;

   return calc;
}

void CalcDestroy(calc_t *calc)
{
    assert(NULL != calc);
    
    StackDestroy(calc->stack_numbers);
    StackDestroy(calc->stack_operators);
    free(calc);
}
status_t CalcPushOP(calc_t *calc, void *op)
{
    char top_op = 0;
    op_t top_operator = {0};

    assert(NULL != calc);
    assert(NULL != op);

    top_op = *(char *)StackPeek(calc->stack_operators);
    top_operator = OPERATORS[(int)top_op];

    if((((op_t *)op)->precendence < top_operator.precendence) && (StackSize(calc->stack_operators) > 0 && (StackSize(calc->stack_numbers) > 1)))
    {
        if(INVALID_EXPRESSION == Fold(calc))
        {
            return INVALID_EXPRESSION;
        }
    }

    StackPush(calc->stack_operators, op);

    return VALID;

}

status_t CalcPushNum(calc_t *calc, void *num)
{
    assert(NULL != calc);
    assert(NULL != num);

    StackPush(calc->stack_numbers, num);

    return VALID;
}

status_t CalcPushOpenParan(calc_t *calc, void *open)
{
    assert(NULL != calc);
    assert(NULL != open);
    
    StackPush(calc->stack_operators, open);

    return VALID;
}

status_t CalcPushCloseParan(calc_t *calc, void *close)
{
    char *top_op = NULL;

    assert(NULL != calc);
    assert(NULL != close);

    if (StackSize(calc->stack_operators) > 0)
        {
            top_op = (char *)StackPeek(calc->stack_operators);

            while ('(' != (int)*top_op)
            {
                if (0 == StackSize(calc->stack_operators))
                {
                    return INVALID_EXPRESSION;
                }

                if (INVALID_EXPRESSION == Fold(calc))
                {
                    return INVALID_EXPRESSION;
                }

                top_op = (char *)StackPeek(calc->stack_operators);
            }

            StackPop(calc->stack_operators);

            return VALID;
        }
    

    return INVALID_EXPRESSION;
}

status_t CalcGetResult(calc_t *calc, void *result)
{
    status_t status = VALID;

    assert(NULL != calc);
    assert(NULL != result);

    while((StackSize(calc->stack_operators) > 0) && (StackSize(calc->stack_numbers) > 1))
    {
        status = Fold(calc);
        if(VALID != status)
        {
            return status;
        }
    }

    if('('== *(char *)StackPeek(calc->stack_operators))
    {
        return INVALID_EXPRESSION;
    }

    *(double *)result = *(double *)StackPeek(calc->stack_numbers);

    return VALID;
}

parser_t *ParserCreate (char *str)
{
    parser_t *parser = (parser_t *)malloc(sizeof(parser_t));

    assert(NULL != str);
    
    if(NULL == parser)
    {
        return NULL;
    }

    parser->calc = CalcCreate();
    if(NULL == parser->calc)
    {
        free(parser);
        return NULL;
    }
    
    parser->current_state = WAIT_NUM;
    parser->str = str;

    return parser;
}

void ParserDestroy(parser_t *parser)
{
    assert(NULL != parser);

    CalcDestroy(parser->calc);
    free(parser);
}

status_t ParserOp(parser_t *parser)
{
    char op = 0;
    
    assert(NULL != parser);
    
    op = *(parser->str);
    if(VALID != CalcPushOP(parser->calc, &op))
    {
       return INVALID_EXPRESSION;
    }
    
    parser->current_state = FSM[parser->current_state][(int)(*parser->str)].next_state;
   
    ++(parser->str);
    if((int)*(parser->str) == ' ')
    {
        ++parser->str;
    }

    return VALID;
}

status_t ParserNum (parser_t *parser)
{
    double num = 0;
    char *start_str = parser->str;
    status_t status = VALID;

    assert(NULL != parser);

    num = strtod(parser->str, &(parser->str));
    if(start_str == parser->str)
    {
        return INVALID_EXPRESSION;
    }

    status = CalcPushNum(parser->calc, &num);
    if(VALID != status)
    {
       return status;
    }
    
    if((int)*(parser->str) == ' ')
    {
        ++parser->str;
    }

    parser->current_state = FSM[parser->current_state][(int)*parser->str].next_state;
    return VALID;
}

status_t ParserOpen(parser_t *parser)
{
    char open = '(';

    assert(NULL != parser);

     if(VALID != CalcPushOpenParan(parser->calc, &open))
    {
       return INVALID_EXPRESSION;
    }
    
    parser->current_state = FSM[parser->current_state][(int)open].next_state;
    ++(parser->str);

    return VALID;
}

status_t ParserClose(parser_t *parser)
{
    char close = ')';

    assert(NULL != parser);

     if(VALID != CalcPushCloseParan(parser->calc, &close))
    {
       return INVALID_EXPRESSION;
    }
    
    parser->current_state = FSM[parser->current_state][(int)close].next_state;
    ++(parser->str);
    if((int)*(parser->str) == ' ')
    {
        ++parser->str;
    }

    return VALID;
}

status_t ParserError(parser_t *parser)
{
    assert(NULL != parser);

    return INVALID_EXPRESSION;
}

status_t ParserDoNothing (parser_t *parser)
{
    assert(NULL != parser);

    return VALID;
}

status_t ParserEOS(parser_t *parser, void *result)
{
    status_t status = VALID;

    assert(NULL != parser);
    assert(NULL != result);

    if(parser->current_state != COMPUTE)
    {
        return INVALID_EXPRESSION;
    }

    status = CalcGetResult(parser->calc, result);

    if('(' == *(char *)StackPeek(parser->calc->stack_operators))
    {
        return INVALID_EXPRESSION;
    }

    return status;
}

static status_t Add (double num1, double num2, double *result)
{
    assert(NULL != result);

    *result = num2 + num1;

    return VALID;
}

static status_t Subtract(double num1, double num2, double *result)
{
    assert(NULL != result);

    *result = num2 - num1;

    return VALID;
}

static status_t Multiply(double num1, double num2, double *result)
{
    assert(NULL != result);

    *result = num2 * num1;

    return VALID;
}

static status_t Divide(double num1, double num2, double *result)
{
    assert(NULL != result);

    if(0 == num1)
    {
        return MATH_ERROR;
    }

    *result = num2 / num1;

    return VALID;
}

static status_t DummyFunc (double num1, double num2, double *result)
{
    assert(NULL != result);

    (void)num1;
    (void)num2;

    return VALID;
}

static status_t Fold(calc_t *calc)
{
    double num1 = 0;
    double num2 = 0;
    char op = 0;
    op_t operator = {0};
    double result = 0;
    status_t status = VALID;

    assert(NULL != calc);

    num1 = *(double *)StackPeek(calc->stack_numbers);
    StackPop(calc->stack_numbers);
    num2 = *(double *)StackPeek(calc->stack_numbers);
    StackPop(calc->stack_numbers);
    op = *(char *)StackPeek(calc->stack_operators);
    StackPop(calc->stack_operators);
    operator = OPERATORS[(int)op];
    status = operator.operation_func(num1, num2, &result);
    if(VALID != status)
    {
        return status;
    }

    
    StackPush(calc->stack_numbers,&result);

    return VALID;
}



/*static double Power (double *num1, double *num2)
{
    double power = 1;

    for(i = 0; i < *num2; ++i)
    {
        power = power * *num1;
    }

    return power;
}*/



