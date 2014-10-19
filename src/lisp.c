/**
 *  @file           lisp.c
 *  @brief          The Lisp Interpreter
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 *  @details
 *
 *  Experimental, small, lisp interpreter.
 *
 *  Meaning of symbols:
 *  i:      input
 *  o:      output
 *  x:      expression
 *  args:   a list of *lisp_evaluated* arguments
 *  nx:     a newly created expression
 *
 **/

#include "lisp.h"
#include <string.h>
#include <assert.h>
#include "mem.h"
#include "gc.h"
#include "sexpr.h"
#include "regex.h"

/** 
 * @brief List of primitive operations, used for initialization of structures 
 *        and function declarations. It uses X-Macros to achieve this job.
 *        See  <https://en.wikipedia.org/wiki/X_Macro> and
 *        <http://www.drdobbs.com/cpp/the-x-macro/228700289>
 **/
#define LIST_OF_PRIMITIVE_OPERATIONS\
        PRIMOP_X("+",        primop_add)\
        PRIMOP_X("-",        primop_sub)\
        PRIMOP_X("*",        primop_prod)\
        PRIMOP_X("/",        primop_div)\
        PRIMOP_X("mod",      primop_mod)\
        PRIMOP_X("car",      primop_car)\
        PRIMOP_X("cdr",      primop_cdr)\
        PRIMOP_X("cons",     primop_cons)\
        PRIMOP_X("nth",      primop_nth)\
        PRIMOP_X("length",   primop_len)\
        PRIMOP_X("=",        primop_numeq)\
        PRIMOP_X("print",    primop_printexpr)\
        PRIMOP_X("scar",     primop_scar)\
        PRIMOP_X("scdr",     primop_scdr)\
        PRIMOP_X("scons",    primop_scons)\
        PRIMOP_X("eqt",      primop_typeeq)\
        PRIMOP_X("reverse",  primop_reverse)\
        PRIMOP_X("system",   primop_system)\
        PRIMOP_X("match",    primop_match)
 
/** @brief built in primitives, static declarations **/
#define PRIMOP_X(STRING, FUNCTION) static expr FUNCTION(expr args, lisp l);
LIST_OF_PRIMITIVE_OPERATIONS
#undef PRIMOP_X

/** @brief initializer table for primitive operations **/
#define PRIMOP_X(STRING, FUNCTION) {STRING, FUNCTION},
static primop_initializers primops[] = {
        LIST_OF_PRIMITIVE_OPERATIONS
        {NULL,       NULL} /* this *has* to be the last entry */
};
#undef PRIMOP_X

/*** interface functions *****************************************************/

/**
 *  @brief          Initialize the lisp interpreter
 *  @return         A fully initialized lisp environment
 **/
lisp lisp_init(void)
{

}

/** 
 *  @brief      Registers a function for use within the lisp environment    
 *  @param      name    functions name
 *  @param      func    function to register.
 *  @param      l       lisp environment to register function in
 *  @return     int     Error code, 0 = Ok, >0 is a failure.
 */
int lisp_register_function(char *name, expr(*func) (expr args, lisp l), lisp l){
}

/** 
 *  @brief    lisp_repl implements a lisp Read-Evaluate-Print-Loop
 *  @param    l an initialized lisp environment
 *  @return   Always zero at the moment
 *
 *  @todo When Error Expression have been properly implemented any
 *        errors that have not been caught should be returned by lisp_repl
 *        or handled by it to avoid multiple error messages being printed
 *        out.
 */
lisp lisp_repl(lisp l)
{
        expr x;
        while (NULL != (x = sexpr_parse(l->i))) {
                x = lisp_eval(x, l->env, l);
                sexpr_print(x, l->o, 0);
                lisp_clean(l);
        }
        return l;
}

/**
 *  @brief          Destroy and clean up a lisp environment
 *  @param          l   initialized lisp environment
 *  @return         void
 **/
void lisp_end(lisp l)
{
}

/**
 *  @brief          Read in an s-expression 
 *  @param          i   Read input from...
 *  @return         A valid s-expression, which might be an error!
 **/
expr lisp_read(io * i)
{
        return sexpr_parse(i);
}

/**
 *  @brief          Print out an s-expression
 *  @param          x   Expression to print
 *  @param          o   Output stream
 *  @return         void
 **/
void lisp_print(expr x, io * o)
{
        sexpr_print(x, o, 0);
        return;
}

/**
 *  @brief          Evaluate an already parsed lisp expression
 *  @param          x   The s-expression to parse
 *  @param          env The environment to lisp_evaluate in
 *  @param          l   The global lisp environment
 *  @return         An lisp_evaluated expression, possibly ready for printing.
 **/
expr lisp_eval(expr x, expr env, lisp l)
{
}

/**
 *  @brief          Garbage collection
 *  @param          l   Lisp environment to mark and collect
 *  @return         void
 **/
void lisp_clean(lisp l)
{
        gc_mark(l->global);
        gc_sweep();
}



/*** primitive operations ****************************************************/

/**macro helpers for primops**/
#define INTCHK_R(EXP)\
  if(S_INTEGER!=((EXP)->type)){\
    SEXPR_PERROR((EXP),"arg != integer");\
    return nil;\
  }

/**Avoid warning in primops**/
#define UNUSED(X)  (void)(X)

/**add a list of numbers**/
static expr primop_add(expr args, lisp l){}

/**subtract a list of numbers from the 1 st arg**/
static expr primop_sub(expr args, lisp l)
{}

/**multiply a list of numbers together**/
static expr primop_prod(expr args, lisp l)
{}

/**divide the first argument by a list of numbers**/
static expr primop_div(expr args, lisp l)
{}

/**arg_1 modulo arg_2**/
static expr primop_mod(expr args, lisp l)
{}

/**car**/
static expr primop_car(expr args, lisp l)
{}

/**cdr**/
static expr primop_cdr(expr args, lisp l)
{}

/**cons**/
static expr primop_cons(expr args, lisp l)
{}

/**NTH element in a list or string**/
static expr primop_nth(expr args, lisp l)
{}

/**length of a list or string**/
static expr primop_len(expr args, lisp l)
{}

/**test equality of the 1st arg against a list of numbers**/
static expr primop_numeq(expr args, lisp l)
{}

/**print**/
static expr primop_printexpr(expr args, lisp l)
{}

/**CAR for strings**/
static expr primop_scar(expr args, lisp l)
{}

/**cdr for strings**/
static expr primop_scdr(expr args, lisp l)
{}

/**cons for strings**/
static expr primop_scons(expr args, lisp l)
{}

/**type equality**/
static expr primop_typeeq(expr args, lisp l)
{}

/**reverse a list or a string**/
static expr primop_reverse(expr args, lisp l)
{}

static expr primop_system(expr args, lisp l)
{}

static expr primop_match(expr args, lisp l)
{}

#undef INTCHK_R
#undef UNUSED

/*****************************************************************************/
