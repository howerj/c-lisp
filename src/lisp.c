/** 
 *  Richard Howe
 *
 *  License: GPL
 *
 *  Experimental, small, lisp interpreter.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "io.h"
#include "mem.h"
#include "sexpr.h"
#include "lisp.h"

static char *usage = "./lisp -hVi <file>";

/** 
 * version should include md5sum calculated from
 *  c and h files, excluding the file it gets put
 *  into. This will be included here.
 *
 *  Generation would be like this:
 *  md5sum *.c *.h | md5sum | more_processing > version/version.h
 *  in the makefile
 */
static char *version = __DATE__ " : " __TIME__ "\n";
static char *help = "\n\
Lisp Interpreter. Richard James Howe\n\
usage:\n\
  ./lisp -hVi <file>\n\
\n\
  -h      Print this help message and exit.\n\
  -V      Print version number and exit.\n\
  -i      Input file.\n\
  <file>  Iff -i given read from instead of stdin.\n\
";

static int getopt(int argc, char *argv[]);

#define car(X)  ((expr)((X)->data.list[0]))
#define cdr(X)  ((expr)((X)->data.list[1]))
#define cddr(X)  ((expr)((X)->data.list[2]))
#define cdddr(X)  ((expr)((X)->data.list[3]))
#define tstlen(X,Y) ((Y)==(X)->len)

static expr mkobj(sexpr_e type, io *e);
expr eval(expr x, expr env, lisp l);
expr apply(expr x, expr env, lisp l);
lisp initlisp(void);
bool primcmp(expr x, char *s, io *e);

static expr nil;

int main(int argc, char *argv[]){
  lisp l;
  expr x,env=NULL;

  /** setup environment */
  l = initlisp();

  if(1<argc){
    if(getopt(argc,argv)){
        fprintf(stderr,"%s\n",usage);
        return 1;
    }
  } else {

  }

  while((x = parse_term(&l->i, &l->e))){
    /*printf("#%p\n",(void*)eval(x,env,l));*/
    x = eval(x,env,l);
    print_expr(x,&l->o,0,&l->e);
    /** TODO:
     * Garbarge collection; everything not marked in the eval
     * gets collected by free_expr
     */
    /*free_expr(x, &l->e);*/
  }

  return 0;
}

/** TODO:
 *    - implement input file option
 *    - --""-- output --""--
 *    - execute on string passed in
 */
static int getopt(int argc, char *argv[]){
  int c;
  if(argc<=1)
    return 0;

  if('-' != *argv[1]++){ /** TODO: Open arg as file */
    return 0;
  }

  while((c = *argv[1]++)){
    switch(c){
      case 'h':
        printf("%s",help);
        return 0;
      case 'V':
        printf("%s",version);
        return 0;
      case 'i':
        break;
      default:
        fprintf(stderr,"unknown option: '%c'\n", c);
        return 1;
    }
  }

  return 0;
}

lisp initlisp(void){ /** initializes the environment, nothing special here */
  lisp l;
  expr global;
  l      = wcalloc(sizeof (lispenv_t),1,NULL);
  global = wcalloc(sizeof (lispenv_t),1,NULL);
  if(!l||!global)
    exit(-1);

  /** set up file I/O and pointers */
  l->i.type     = file_in;
  l->i.ptr.file = stdin;
  l->o.type     = file_out;
  l->o.ptr.file = stdout;
  l->e.type     = file_out ;
  l->e.ptr.file = stderr;
  l->current    = NULL;
  l->global     = global;

  global->type  = S_LIST;
  nil = mkobj(S_NIL,&l->e);
  return l;
}


static expr mkobj(sexpr_e type,io *e){
  expr x;
  x = wcalloc(sizeof(sexpr_t), 1,e);
  x->len = 0;
  x->type = type;
  return x;
}

bool primcmp(expr x, char *s, io *e){
  if(NULL == (car(x)->data.symbol)){
    report("null passed to primcmp!");
    abort();
  }
  return !strcmp(car(x)->data.symbol,s);
}

expr eval(expr x, expr env, lisp l){
  unsigned int i;
  io *e = &l->e;
  expr ne; /* new expression */

  if(NULL==x){
    report("passed null!");
  }

  switch(x->type){
    case S_LIST: /** most of eval goes here! */
      /** what should ((quote +) 2 3) do ?*/
      if(tstlen(x,0)) /* () */
        return nil;
      if(S_SYMBOL==car(x)->type){
        if(primcmp(x,"if",e)){ /* (if test conseq alt) */
          if(!tstlen(x,4)){
            report("special form 'if', expected list of size 4");
            return nil;
          }
          if(nil == eval(cdr(x),env,l)){
            return eval(cdddr(x),env,l);
          } else {
            return eval(cddr(x),env,l);
          }
        } else if (primcmp(x,"begin",e)){ /* (begin exp ... ) */
          if(tstlen(x,1)){
            return nil;
          }
          for (i = 1; i < x->len - 1; i++){
            eval((expr)(x->data.list[i]),env,l);
          }
          return eval((expr)(x->data.list[i]),env,l);
        } else if (primcmp(x,"quote",e)){ /* (quote exp) */
          if(!tstlen(x,2)){
            report("special form 'quote', expected list of size 2");
            return nil;
          }
          return cdr(x);
        } else if (primcmp(x,"set",e)){
          if(!tstlen(x,3)){
            report("special form 'set', expected list of size 3");
            return nil;
          }
        } else if (primcmp(x,"define",e)){
        } else if (primcmp(x,"lambda",e)){
        } else {
          /** symbol look up and apply */
          return apply(x,env,l);
        }
      } else {
        report("cannot apply");
        print_expr(car(x),&l->o,0,e);
      }

      /*for (i = 0; i < x->len; i++){
        print_expr((expr)(x->data.list[i]), &l->o , 0,e);
      }*/
      break; 
    case S_SYMBOL:
      /*if symbol found, return it, else error; unbound symbol*/
    case S_FILE: /* to implement */
      break; 
    case S_NIL:
    case S_STRING:
    case S_PROC:
    case S_INTEGER:
    case S_PRIMITIVE:
      return x; 
    default:
      report("Serious error, unknown type");
      abort();
  }

  printf("should never get here\n");
  return x;
}

expr apply(expr x, expr env, lisp l){
  return x;
}
