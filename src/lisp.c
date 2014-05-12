/**
 *  @file           lisp.c
 *  @brief          The Lisp Interpreter; Lispy Space Princess
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v3.0
 *  @email          howe.r.j.89@gmail.com
 *  @details
 *
 *  Experimental, small, lisp interpreter.
 *
 *  Meaning of symbols:
 *  i:      input
 *  o:      output
 *  e:      standard error output
 *  x:      expression
 *  args:   a list of *evaluated* arguments
 *  ne:     a newly created expression
 *
 *  @todo Better error reporting, report() should be able to
 *        (optionally) print out expressions
 *  @todo Better error handling; a new primitive type should be made
 *        for it, one that can be caught.
 *  @todo Make the special forms less special!
 *  @todo Make more primitives and mechanisms for handling things:
 *         - Register internal functions as lisp primitives.
 *         - random, seed
 *         - cons,listlen,reverse, more advanced lisp manipulation funcs ...
 *         - eq = > < <= >=
 *         - string manipulation and regexes; tr, sed, //m, pack, unpack, ...
 *         - type? <- returns type of expr
 *         - type coercion and casting
 *         - file manipulation and i/o: read, format, 
 *           read-char read-line write-string, ...
 *         - max, min, abs, ...
 *         - Error handling and recovery
 *         - not, and, or, logical functions as well!
 *         - comment; instead of normal comments, comments and the
 *         unevaluated sexpression could be stored for later retrieval
 *         and inspection, keeping the source and the runnning program
 *         united.
 *         - modules; keywords for helping in the creation of modules
 *         and importing them.
 **/

#include <string.h> /* strcmp, strlen, strcat */
#include "type.h"   /* includes some std c library headers */
#include "io.h"
#include "mem.h"
#include "sexpr.h"
#include "lisp.h"

/** macro helpers **/
#define car(X)      ((X)->data.list[0])
#define cadr(X)     ((X)->data.list[1]) 
#define caddr(X)    ((X)->data.list[2])
#define cadddr(X)   ((X)->data.list[3])
#define nth(X,Y)    ((X)->data.list[(Y)])
#define tstlen(X,Y) ((Y)==((X)->len))

#define procargs(X) car(X)
#define proccode(X) cadr(X)
#define procenv(X)  caddr(X)

/** global-to-file special objects **/
static expr nil, tee;

/** functions necessary for the intepreter **/
static expr mkobj(sexpr_e type, io *e);
static expr mksym(char *s, io *e);
static expr mkprimop(expr (*func)(expr args, lisp l),io *e);
static expr mkproc(expr args, expr code, expr env, io *e);
static expr evlis(expr x,expr env,lisp l);
static expr apply(expr proc, expr args, lisp l); /*add env for dynamic scope?*/
static expr find(expr env, expr x, io *e);
static expr extend(expr sym, expr val, expr env, io *e);
static expr extendprimop(char *s, expr (*func)(expr args, lisp l), expr env, io *e);
static expr extensions(expr env, expr syms, expr vals, io *e);
static bool primcmp(expr x, const char *s, io *e);

/** built in primitives **/
static expr primop_add(expr args, lisp l);
static expr primop_sub(expr args, lisp l);
static expr primop_prod(expr args, lisp l);
static expr primop_div(expr args, lisp l);
static expr primop_mod(expr args, lisp l);
static expr primop_car(expr args, lisp l);
static expr primop_cdr(expr args, lisp l);
static expr primop_cons(expr args, lisp l);
static expr primop_nth(expr args, lisp l);
static expr primop_len(expr args, lisp l);
static expr primop_numeq(expr args, lisp l);
static expr primop_printexpr(expr args, lisp l);

/*** interface functions *****************************************************/

/**
 *  @brief          Initialize the lisp interpreter
 *  @param          void
 *  @return         A fully initialized lisp environment
 **/
lisp initlisp(void){
  lisp l;
  expr global;
  l      = wcalloc(1,sizeof (lispenv_t),NULL);
  global = wcalloc(1,sizeof (sexpr_t),NULL);
  if((NULL==l)||(NULL==global))
    exit(EXIT_FAILURE);

  l->i = wcalloc(1,sizeof (io),NULL);
  l->o = wcalloc(1,sizeof (io),NULL);
  l->e = wcalloc(1,sizeof (io),NULL);

  if((NULL==l->i)||(NULL==l->o)||(NULL==l->e))
    exit(EXIT_FAILURE);

  /* set up file I/O and pointers */
  l->i->type     = file_in;
  l->i->ptr.file = stdin;
  l->o->type     = file_out;
  l->o->ptr.file = stdout;
  l->e->type     = file_out ;
  l->e->ptr.file = stderr;
  l->global      = global;

  global->type  = S_LIST;
  nil = mkobj(S_NIL,l->e);
  tee = mkobj(S_TEE,l->e);

  /* internal symbols */
  extend(mksym("nil", l->e),nil,l->global,l->e);
  extend(mksym("t", l->e),tee,l->global,l->e);

  /* normal forms, kind of  */
  extendprimop("+",       primop_add,       l->global, l->e);
  extendprimop("-",       primop_sub,       l->global, l->e);
  extendprimop("*",       primop_prod,      l->global, l->e);
  extendprimop("/",       primop_div,       l->global, l->e);
  extendprimop("mod",     primop_mod,       l->global, l->e);
  extendprimop("car",     primop_car,       l->global, l->e);
  extendprimop("cdr",     primop_cdr,       l->global, l->e);
  extendprimop("cons",    primop_cons,      l->global, l->e);
  extendprimop("nth",     primop_nth,       l->global, l->e);
  extendprimop("length",  primop_len,       l->global, l->e);
  extendprimop("=",       primop_numeq,     l->global, l->e);
  extendprimop("print",   primop_printexpr, l->global, l->e);

  return l;
}

/**
 *  @brief          Evaluate an already parsed lisp expression
 *  @param          x   The s-expression to parse
 *  @param          env The environment to evaluate in
 *  @param          l   The global lisp environment
 *  @return         An evaluate expression, possibly ready for printing.
 **/
expr eval(expr x, expr env, lisp l){
  unsigned int i;
  io *e = l->e;

  if(NULL==x){
    report("passed null!");
    abort();
  }

  switch(x->type){
    case S_LIST:
      if(tstlen(x,0)) /* () */
        return nil;
      if(S_SYMBOL==car(x)->type){
        if(primcmp(x,"if",e)){ /* (if test conseq alt) */
          if(!tstlen(x,4)){
            report("if: argc != 4");
            return nil;
          }
          if(nil == eval(cadr(x),env,l)){
            return eval(cadddr(x),env,l);
          } else {
            return eval(caddr(x),env,l);
          }
        } else if (primcmp(x,"begin",e)){ /* (begin exp ... ) */
          if(tstlen(x,1)){
            return nil;
          }
          for (i = 1; i < x->len - 1; i++){
            eval(nth(x,i),env,l);
          }
          return eval(nth(x,i),env,l);
        } else if (primcmp(x,"quote",e)){ /* (quote exp) */
          if(!tstlen(x,2)){
            report("quote: argc != 1");
            return nil;
          }
          return cadr(x);
        } else if (primcmp(x,"set",e)){
          expr ne;
          if(!tstlen(x,3)){
            report("set: argc != 2");
            return nil;
          }
          ne = find(env,cadr(x),l->e);
          if(nil == ne){
            return nil;
          }
          ne->data.list[1] = eval(caddr(x),env,l);
          return cadr(ne);
        } else if (primcmp(x,"define",e)){ 
          { 
            expr ne;
            /*@todo if already defined, or is an internal symbol, report error */
            if(!tstlen(x,3)){
              report("define: argc != 2");
              return nil;
            }/*@warning the proc stuff is a hack*/
            ne = extend(cadr(x),eval(caddr(x),env,l),l->global,e);
            if(S_PROC == ne->type){
              extend(cadr(x),ne,procenv(ne),e);
            }
            return ne;
          }
        } else if (primcmp(x,"lambda",e)){
          if(!tstlen(x,3)){
            report("lambda: argc != 2");
            return nil;
          }
          return mkproc(cadr(x),caddr(x),env,e);
        } else {
          return apply(eval(car(x),env,l),evlis(x,env,l),l);
        }
      } else {
        report("cannot apply");
        print_expr(car(x),l->o,0,e);
      }
      break;
    case S_SYMBOL:/*if symbol found, return it, else error; unbound symbol*/
      {
        expr ne = find(env,x,l->e);
        if(nil == ne){
          return nil;
        }
        return cadr(find(env,x,l->e));
      }
    case S_FILE: /* to implement */
      report("file type unimplemented");
      return nil;
    case S_NIL:
    case S_TEE:
    case S_STRING:
    case S_PROC:
    case S_INTEGER:
    case S_PRIMITIVE:
      return x;
    default:
      report("Serious error, unknown type"); 
      abort();
  }

  report("should never get here");
  return x;
}

/*** internal functions ******************************************************/

/** find a symbol in a special type of list **/
static expr find(expr env, expr x, io *e){
  /** @todo make this function more robust **/
  unsigned int i;
  char *s = x->data.symbol;
  for(i = env->len - 1; i != 0 ; i--){ /** reverse search order **/
    if(!strcmp(car(nth(env,i))->data.symbol, s)){
      return nth(env,i);
    }
  }
  if(!strcmp(car(nth(env,0))->data.symbol, s)){
      return nth(env,0);
  }
  report("unbound symbol");
  return nil;
}

/** extend a lisp environment **/
static expr extend(expr sym, expr val, expr env, io *e){
  expr ne = mkobj(S_LIST,e);
  append(ne,sym,e);
  append(ne,val,e);
  append(env,ne,e);
  return val;
}

static expr extendprimop(char *s, expr (*func)(expr args, lisp l), expr env, io *e){
  return extend(mksym(s,e), mkprimop(func,e), env,e);
}


/** make new object **/
static expr mkobj(sexpr_e type,io *e){
  expr ne;
  ne = wcalloc(sizeof(sexpr_t), 1,e);
  ne->len = 0;
  ne->type = type;
  return ne;
}

/** make a new symbol **/
static expr mksym(char *s,io *e){
  expr ne;
  ne = mkobj(S_SYMBOL,e);
  ne->len = strlen(s);
  ne->data.symbol = s;
  return ne;
}

/** make a new primitive **/
static expr mkprimop(expr (*func)(expr args, lisp l),io *e){
  expr ne;
  ne = mkobj(S_PRIMITIVE,e);
  ne->data.func = func;
  return ne;
}

/** make a new process **/
static expr mkproc(expr args, expr code, expr env, io *e){
  /** 
   *  @todo check all args are symbols, clean this up! 
   *  @warning garbage collection is going to have to
   *  deal with the new environment *somehow*
   **/
  expr ne, nenv;
  ne = mkobj(S_PROC,e);
  append(ne,args,e);
  append(ne,code,e);
  nenv = mkobj(S_LIST,e);
  /** @todo turn into mklist **/
  nenv->data.list = wmalloc(env->len*sizeof(expr),e);
  memcpy(nenv->data.list,env->data.list,(env->len)*sizeof(expr));
  nenv->len = env->len;
  /****************************/
  append(ne,nenv,e);
  return ne;
}

/** compare a symbols name to a string **/
static bool primcmp(expr x, const char *s, io *e){
  if(NULL == (car(x)->data.symbol)){
    report("null passed to primcmp!");
    abort();
  }
  return !strcmp(car(x)->data.symbol,s);
}

/** evaluate a list **/
static expr evlis(expr x,expr env,lisp l){
  unsigned int i;
  expr ne;
  ne = mkobj(S_LIST,l->e);
  for(i = 1/*skip 0!*/; i < x->len; i++){
    append(ne,eval(nth(x,i),env,l),l->e);
  }
  return ne;
}

/** extend a enviroment with symbol/value pairs **/
static expr extensions(expr env, expr syms, expr vals, io *e){
  unsigned int i;
  if(0 == vals->len){
    return nil;
  }
  for(i = 0; i < syms->len; i++){
    extend(nth(syms,i),nth(vals,i),env,e);
  }
  return env;
}

/** apply a procedure over arguments given an environment **/
static expr apply(expr proc, expr args, lisp l){
  io *e = l->e;
  expr nenv;
  if(S_PRIMITIVE == proc->type){
    return (proc->data.func)(args,l);
  }
  if(S_PROC == proc->type){
    if(args->len != procargs(proc)->len){
      report("expected number of args incorrect");
      return nil;
    }
    nenv = extensions(procenv(proc),procargs(proc),args,l->e);
    return eval(proccode(proc),nenv,l);
  }

  report("Cannot apply expression");
  return nil;
}

/*** primitive operations ****************************************************/

static expr primop_add(expr args, lisp l){
  io *e = l->e;
  unsigned int i;
  expr ne;
  ne = mkobj(S_INTEGER,e);
  if(0 == args->len)
    return nil;
  for(i = 0; i < args->len; i++){
    if(S_INTEGER!=nth(args,i)->type){
      report("not an integer type"); 
      return nil;
    }
    ne->data.integer+=(nth(args,i)->data.integer);
  }
  return ne;
}

static expr primop_sub(expr args, lisp l){
  io *e = l->e;
  unsigned int i;
  expr ne;
  ne = mkobj(S_INTEGER,e);
  if(0 == args->len)
    return nil;
  ne = nth(args,0);
  for(i = 1; i < args->len; i++){
    if(S_INTEGER!=nth(args,i)->type){
      report("not an integer type"); 
      return nil;
    }
    ne->data.integer-=(nth(args,i)->data.integer);
  }
  return ne;
}

static expr primop_prod(expr args, lisp l){
  io *e = l->e;
  unsigned int i;
  expr ne;
  ne = mkobj(S_INTEGER,e);
  if(0 == args->len)
    return nil;
  ne = nth(args,0);
  for(i = 1; i < args->len; i++){
    if(S_INTEGER!=nth(args,i)->type){
      report("not an integer type"); 
      return nil;
    }
    ne->data.integer*=(nth(args,i)->data.integer);
  }
  return ne;
}

static expr primop_div(expr args, lisp l){
  io *e = l->e;
  unsigned int i,tmp;
  expr ne = mkobj(S_INTEGER,e);
  if(0 == args->len)
    return nil;
  ne = nth(args,0);
  for(i = 1; i < args->len; i++){
    if(S_INTEGER!=nth(args,i)->type){
      report("not an integer type"); 
      return nil;
    }
    tmp = nth(args,i)->data.integer;
    if(tmp){
      ne->data.integer/=tmp;
    }else{
      report("attempted /0");
      return nil;
    }
  }
  return ne;
}

static expr primop_mod(expr args, lisp l){
  io *e = l->e;
  unsigned int tmp;
  expr ne = mkobj(S_INTEGER,e);
  if(2!=args->len){
    report("mod: argc != 2");
    return nil;
  }
  if((car(args)->type != S_INTEGER) || (cadr(args)->type != S_INTEGER)){
    report("not an integer type");
    return nil;
  }
  tmp = cadr(args)->data.integer;
  if(0 == tmp){
    report("mod: 0/");
    return nil;
  }
  ne->data.integer = car(args)->data.integer % tmp;

  return ne;
}

static expr primop_car(expr args, lisp l){
  io *e = l->e;
  expr a1 = car(args);
  if((S_LIST != a1->type) && (S_STRING != a1->type)){
    report("args != list || string");
    return nil;
  }
  if(1!=args->len){
    report("car: argc != 1");
    return nil;
  }
  if(S_LIST == a1->type){
    return car(a1);
  } else { /*must be string*/
      expr ne = mkobj(S_STRING,e);
      ne->data.string = wcalloc(sizeof(char),a1->len?2:0,e);
      ne->data.string[0] = a1->data.string[0];
      ne->len = 1;
      return ne;
  }
}

static expr primop_cdr(expr args, lisp l){
  io *e = l->e;
  expr ne = mkobj(S_LIST,e), carg = car(args);
  if(((S_STRING != carg->type) && (S_LIST != carg->type)) || (1>=carg->len)){
    return nil;
  }
  /** @warning This should be rewritten to make it
   *  play nice with any future garbage collection
   *  efforts
   **/
  if(S_LIST == carg->type){
    ne->data.list = carg->data.list + 1;
  } else { /*must be a string*/
    ne->type = S_STRING;
    ne->data.string = wcalloc(sizeof(char),carg->len,e);/*not len + 1*/
    strcpy(ne->data.string,carg->data.string + 1);
  }
  ne->len = carg->len - 1;
  return ne;
}

static expr primop_cons(expr args, lisp l){
  io *e = l->e;
  expr ne = mkobj(S_LIST,e),prepend,list;
  if(2!=args->len){
    report("cons: argc != 2");
    return nil;
  }
  prepend = car(args);
  list = cadr(args);
  if(S_NIL == list->type){
    append(ne,prepend,e);
  } else if (S_LIST == list->type){
    /** @todo turn into mklist **/
    ne->data.list = wmalloc((list->len + 1)*sizeof(expr),e);
    car(ne) = prepend;
    memcpy(ne->data.list + 1,list->data.list,(list->len)*sizeof(expr));
    ne->len = list->len + 1;
    /****************************/
  } if((S_STRING == list->type) && (S_STRING == prepend->type)){
    ne->type = S_STRING;
    ne->data.string = wcalloc(sizeof(char),(list->len)+(prepend->len)+1,e);
    ne->len = list->len + prepend->len;
    strcpy(ne->data.string,prepend->data.string);
    strcat(ne->data.string,list->data.string);
  } else {
    append(ne,prepend,e);
    append(ne,list,e);
  }
  return ne;
}

static expr primop_nth(expr args, lisp l){
  /** @todo implement version for string types **/
  io *e = l->e;
  cell_t i;
  expr a1,a2;
  if(2 != args->len){
    report("nth: argc != 2");
    return nil;
  }
  a1 = car(args);
  a2 = cadr(args);
  if(S_INTEGER != a1->type){
    report("nth: arg 1 != integer");
    return nil;
  }
  if((S_LIST != a2->type) && (S_STRING != a2->type)){
    report("nth: arg 2 != list || string");
    return nil;
  }

  i = car(args)->data.integer;
  if(0 > i){
    i = a2->len + i; 
  }

  if((unsigned)i > a2->len){
    return nil;
  }

  if(S_LIST == a2->type){
    return nth(a2,i);
  } else { /*must be string*/
    {
      expr ne = mkobj(S_STRING,e);
      ne->data.string = wcalloc(sizeof(char),2,e);
      ne->data.string[0] = a2->data.string[i];
      ne->len = 1;
      return ne;
    }
  }
  return nil;
}

static expr primop_len(expr args, lisp l){
  io *e = l->e;
  expr a1, ne = mkobj(S_INTEGER,e);
  if(1 != args->len){
    report("len: argc != 1");
    return nil;
  }
  a1 = car(args);
  if((S_LIST != a1->type) && (S_STRING != a1->type)){
    report("len: arg 2 != list || string");
    return nil;
  }
  ne->data.integer = a1->len;
  return ne;
}

static expr primop_numeq(expr args, lisp l){
  io *e = l->e;
  unsigned int i;
  expr ne = mkobj(S_INTEGER,e);
  if(0 == args->len)
    return nil;
  if(1 == args->len)
    return tee;
  ne = nth(args,0);
  for(i = 1; i < args->len; i++){
    if(S_INTEGER!=nth(args,i)->type){
      report("not an integer type"); 
      return nil;
    }
    if(ne->data.integer != (nth(args,i)->data.integer)){
      return nil;
    }
    ne->data.integer = (nth(args,i)->data.integer);
  }
  return tee;
}

static expr primop_printexpr(expr args, lisp l){
  /* @todo if arg = 1, treat as I/O, else normal out */
  print_expr(args,l->o,0,l->e);
  return args;
}

/*****************************************************************************/
