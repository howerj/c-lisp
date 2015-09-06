/** @file       liblisp.h
 *  @brief      A minimal lisp interpreter and utility library
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *
 *  The API documentation for the lisp interpreter is kept here, so if you have
 *  access to this header then you (should) have all the documentation needed
 *  to use this library.
 *
 *  See <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> for the
 *  license.
 *
 *  Do not pass NULL to any of these functions unless they specifically mention
 *  that you can. They "assert()" their inputs.
**/
#ifndef LIBLISP_H
#define LIBLISP_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

/************************* structs and opaque types **************************/

/** These are all the types needed for the interpreter and associated utility
 *  functions. They should be opaque types if possible to hide the
 *  implementation details. **/

typedef double lfloat;                /**< float type used by this lisp*/
typedef struct io io;                 /**< generic I/O to files, strings, ...*/
typedef struct hashtable hashtable;   /**< standard hash table implementation */
typedef struct tr_state tr_state;     /**< state for translation functions */
typedef struct cell cell;             /**< a lisp object, or "cell" */
typedef struct lisp lisp;             /**< a full lisp environment */
typedef cell *(*subr)(lisp *, cell*); /**< lisp primitive operations */
typedef void *(*hash_func)(const char *key, void *val); /**< for hash foreach */

typedef void (*ud_free)(cell*);       /**< function to free a user type*/
typedef void (*ud_mark)(cell*);       /**< marking function for user types*/
typedef int  (*ud_equal)(cell*, cell*);  /**< equality function for user types*/
typedef int  (*ud_print)(io*, unsigned, cell*); /**< print out user def types*/

/**@brief This is a prototype for the (optional) line editing functionality
 *        that the REPL can use. The editor function should accept a prompt
 *        to print out and return a line of text to be evaluated by the
 *        REPL.**/
typedef char *(*editor_func)(const char *); 

typedef struct {
        char *start; /**< where the match started*/
        char *end;   /**< where the match ended*/
        int result;  /**< the result, -1 on error, 0 on no match, 1 on match*/
} regex_result; /**< a structure representing a regex result*/

typedef enum tr_errors { 
        TR_OK      =  0, /**< no error*/
        TR_EINVAL  = -1, /**< invalid mode sequence*/
        TR_DELMODE = -2  /**< set 2 string should be NULL in delete mode*/
} tr_errors; /**< possible error values returned by tr_init*/

/************************** useful functions *********************************/

/* This module mostly has string manipulation functions to make processing
 * them a little more comfortable in C. The interpreter uses them internally
 * but they is little reason to hide them. */

/**@brief print out a neatly formatted error message to stderr then exit(-1).
 * @param msg  message to print out
 * @param file file name to print out, use __FILE__
 * @param line line number to print out, use __LINE__**/
void pfatal(char *msg, char *file, long line);

/** @brief   lstrdup for where it is not available, despite what POSIX thinks
 *           it is not OK to stick random junk into the C-library.  
 *  @param   s  the string to duplicate
 *  @return  char* NULL on error, a duplicate string that can be freed
 *                 otherwise **/
char *lstrdup(const char *s);

/** @brief lstrcatend is like "strcat" from "string.h", however it returns the
 *         end of the new string instead instead of the start of it which is
 *         much more useful.
 *  @param  dest a block of memory containing a ASCII NUL terminated string,
 *               it must be capable of hold the "dest" string as well as the
 *               "src" string, plus a single ASCII NUL character
 *  @param  src  An ASCII NUL terminated string to copy into dest
 *  @return char* The end of the new string**/
char *lstrcatend(char *dest, const char *src);

/** @brief   a dumb, but cleverly simple, regex matcher, from
 *           <http://c-faq.com/lib/regex.html>, but modified slightly, by
 *           Arjan Kenter, copied below:
 *
 *           int match(char *pat, char *str) {
 *                   switch(*pat) {
 *                   case '\0': return !*str;
 *                   case '*':  return match(pat+1, str) ||
 *                                          *str && match(pat, str+1);
 *                   case '?':  return *str && match(pat+1, str+1);
 *                   default:   return *pat == *str && match(pat+1, str+1);
 *                   }
 *           }
 *
 *           Another regex matcher that is a little longer and more powerful
 *           can be found at: 
 *
 *   <http://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html>
 *
 *           Which is a little more powerful but longer.
 *
 *           This pattern matcher is a little different, to the one shown
 *           above, it has error handling and has a different syntax.
 *
 *           'c'  An arbitrary character
 *           '*'  Any string of characters
 *           '.'  Any character
 *
 *           And allows for characters to be escaped with the backslash
 *           character, "\*" and "\." match only a single asterisk and
 *           period respectively.
 *
 *  @param   pat  a pattern to match
 *  @param   str  the string to match on
 *  @return  int  1 == match, 0 == no match, -1 == error **/
int match(char *pat, char *str);

/* @brief  A very small and simple regular expression engine adapted from here:
 *         <http://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html>
 *         It is more powerful than the simpler "match" function whilst still
 *         being very small.
 *
 *         Supports:
 *
 *         'c'  Matched any specific character
 *         '.'  Matches any character
 *         '^'  Anchors search at the beginning of the string
 *         '$'  Anchors search to the end of the string
 *         '?'  Zero or one of the previous character
 *         '*'  Zero or more of the previous character
 *         '+'  One or more of the previous character 
 *         '\\' Escape the next character
 *
 *         This adaption of the original is probably quite buggy.
 *
 * @param  regexp NUL terminated regular expression pattern to search for
 * @param  text   NUL terminated string to perform search in
 * @return int -1 on error, 0 on no match, 1 on match**/
regex_result regex_match(char *regexp, char *text);

/** @brief   a hash algorithm by Dan Bernstein, see
 *           <http://www.cse.yorku.ca/~oz/hash.html> for more information.
 *  @param   s      the string to hash
 *  @param   len    length of s
 *  @return  uint32_t      the resulting hash **/
uint32_t djb2(const char *s, size_t len);

/** @brief   get a line text from a file
 *  @param   in    an input file
 *  @return  char* a line of input, without the newline**/
char *getaline(FILE *in);

/** @brief   gets a record delimited by "delim"
 *  @param   in    an input file
 *  @param   delim the delimiter, can be EOF
 *  @return  char* a record of text, minus the delimiter**/
char *getadelim(FILE *in, int delim);

/** @brief concatenate a variable amount of strings with a separator
 *         inbetween them
 *  @param separator    a string to insert between the other strings
 *  @param first        the first string to concatenate
 *  @param ...          a list of strings, with a NULL sentinel at the end
 *  @return char *      a NUL terminated string that can be freed**/
char *vstrcatsep(const char *separator, const char *first, ...);

/** @brief    Calculate the binary logarithm, for more efficient examples see
 *            http://graphics.stanford.edu/~seander/bithacks.html or
 *            "Bit Twiddling Hacks by Sean Eron Anderson"
 *  @param    v         Value to calculate the binary logarithm of
 *  @return   uint8_t   Binary log**/
uint8_t binlog(unsigned long long v);

/** @brief "xorshift" pseudo random number generator
 *  https://en.wikipedia.org/wiki/Xorshift#Xorshift.2B
 *  http://xorshift.di.unimi.it/xorshift128plus.c
 *  http://xorshift.di.unimi.it/xorshift-1.1.1.tgz 
 *  @param  s PRNG internal state
 *  @return A pseudo random number **/
uint64_t xorshift128plus(uint64_t s[2]);

/** @brief return the "balance" of a string of parens, this function
 *         takes into account strings. 
 *
 *  For example:
 *
 *         balance("(((") == 3;
 *         balance(")))") == -3;
 *         balance("()") == 0;
 *         balance("") == 0;
 *         balance("\"( Hello World\\" ( \" ))") == 2;
 *
 *  @param sexpr string to count balance in 
 *  @return int  positive more '(', negative more ')', zero == balanced**/ 
int balance(const char *sexpr);

/************************** hash library *************************************/

/* A small hash library implementation, all state needed for it is held 
 * within the table. */

/** @brief   create new instance of a hash table
 *  @param   len number of buckets in the table
 *  @return  hashtable* initialized hash table or NULL**/
hashtable *hash_create(size_t len);

/** @brief   destroy and invalidate a hash table, this will not attempt to
 *           free the values associated with a key.
 *  @param   h table to destroy or NULL**/
void hash_destroy(hashtable *h);

/** @brief   insert a value into an initialized hash table
 *  @param   ht    table to insert key-value pair into
 *  @param   key   key to associate with a value
 *  @param   val   value to lookup
 *  @return  int   0 on success, < 0 on failure**/
int hash_insert(hashtable *ht, const char *key, void *val);

/** @brief   look up a key in a table
 *  @param   table table to look for value in
 *  @param   key   a key to look up a value with
 *  @return  void* either the value you were looking for a NULL**/
void *hash_lookup(hashtable *table, const char *key);

/** @brief  Apply "func" on each key-val pair in the hash table until
 *          the function returns non-NULL or it has been applied to all
 *          the key-value pairs
 *  @param  h      table to apply func on
 *  @param  func   function to apply
 *  @return void*  the result of the first non-NULL function application**/
void *hash_foreach(hashtable *h, hash_func func);

/** @brief   print out the key-value pairs in a table, keys are
 *           printed as strings, values as the pointer value
 *  @param   h table to print out**/
void hash_print(hashtable *h);

/******************* I/O for reading/writing to strings or files *************/

/* A generic set of functions for reading and writing to files, but also
 * to other things like strings, NULL devices, etcetera. It is more limited
 * than the stdio library when operating on files. For example the IO channel
 * or ports used here can only be either outputs or inputs, never both*/

/** @brief  check whether IO channel is input channel
 *  @param  i   IO channel to check
 *  @return int non zero if channel is input channel**/
int is_isin(io *i);

/** @brief  check whether IO channel is an output channel
 *  @param  o   IO channel to check
 *  @return int non zero if channel is output channel**/
int io_is_out(io *o);

/** @brief  check whether IO channel is using a file
 *  @param  f   IO channel to check
 *  @return int non zero if channel is using a file for IO **/
int io_isfile(io *f);

/** @brief  check whether IO channel is using a string
 *  @param  s   IO channel to check
 *  @return int non zero if channel is using a string for IO **/
int io_is_string(io *s);

/** @brief  check whether IO channel is using a null IO device
 *  @param  n   IO channel to check
 *  @return int non zero if channel is using a null IO device for IO **/
int io_isnull(io *n);

/** @brief  get a char from I/O stream
 *  @param  i   I/O stream, must be readable
 *  @return int same as getc, char on success EOF on failure**/
int io_getc(io *i);

/** @brief  single character of put-back into I/O stream
 *  @param  c   character to put back
 *  @param  i   I/O stream, must be set up for reading
 *  @return int same character as you put in, EOF on failure**/
int io_ungetc(char c, io *i);

/** @brief  write a single char to an I/O stream
 *  @param  c    character to write
 *  @param  o    I/O stream, must be set up for writing
 *  @return int  same character as you put in, EOF on failure**/
int io_putc(char c, io *o);

/** @brief  write a string to an I/O stream
 *  @param  s    string to write
 *  @param  o    I/O stream, must be setup for writing
 *  @return int  >= 0 is success, < 0 is failure**/
int io_puts(const char *s, io *o);

/** @brief  get a line from an I/O stream
 *  @param  i   I/O stream, must be set up for reading
 *  @return char* a line of input, minus the newline character**/
char *io_getline(io *i);

/** @brief  get a record delimited by 'delim' from an I/O stream
 *  @param  i      I/O stream, must be set up for reading
 *  @param  delim  A character delimiter, can be EOF
 *  @return char*  A character delimited record**/
char *io_getdelim(io *i, int delim);

/** @brief  print an integer to an I/O stream
 *  @param  d   an integer!
 *  @param  o   I/O stream, must be set up for writing 
 *  @return int 0 on success, EOF on failure**/
int io_printd(intptr_t d, io *o);

/** @brief  print a floating point number to an I/O stream
 *  @param  f   a floating point number
 *  @param  o   I/O stream, must be set up for writing 
 *  @return int 0 on success, EOF on failure**/
int io_printflt(double f, io * o);

/** @brief   read from a string
 *  @param   sin string to read from, ASCII nul terminated
 *  @return  io* an initialized I/O stream (for reading) or NULL**/
io *io_sin(char *sin);

/** @brief  read from a file
 *  @param  fin an already opened file handle, opened with "r" or "rb"
 *  @return io* an initialized I/O stream (for reading) of NULL**/
io *io_fin(FILE *fin);

/** @brief  write to a string
 *  @param  out string to write to
 *  @param  len  length of string
 *  @return io*  an initialized I/O stream (for writing) or NULL**/
io *io_sout(char *out, size_t len);

/** @brief  write to a file
 *  @param  fout an already opened file handle, opened with "w" or "wb"
 *  @return io*  an initialized I/O stream (for writing) or NULL**/
io *io_fout(FILE *fout);

/** @brief  close a file, the stdin, stderr and stdout file streams
 *          will not be closed if associated with this I/O stream
 *  @param  close I/O stream to close
 *  @return int   0 on success, EOF on error**/
int io_close(io *close);

/** @brief  test whether an I/O stream is at the end of the file
 *  @param  f    I/O stream to test
 *  @return int  !0 if EOF marker is set, 0 otherwise**/
int io_eof(io *f);

/** @brief  flush an I/O stream
 *  @param  f    I/O stream to flush
 *  @return int  EOF on failure, 0 otherwise**/
int io_flush(io *f);

/** @brief  return the file position indicator of an I/O stream
 *  @param  f    I/O stream to get position from
 *  @return int  less than zero on failure, file position otherwise**/
long io_tell(io *f);

/** @brief  set the position indicator of an I/O stream
 *  @param  f       I/O stream to set position
 *  @param  offset  offset to add to origin
 *  @param  origin  origin to do set the offset relative to
 *  @return int     -1 on error, 0 otherwise **/
int io_seek(io *f, long offset, int origin);

/** @brief  Test whether an error occurred with an I/O stream
 *  @param  f    I/O error to test
 *  @return int  non zero on error**/
int io_error(io *f);

/** @brief turn on colorization for output channel, non of the
 *         io primitives use this (such as io_putc, io_puts, etc),
 *         but other functions that have access to the internals
 *         do, such as the lisp interpreter.
 *  @param out      output channel to set colorize on
 *  @param color_on non zero for true (color on), zero for false**/
void io_color(io *out, int color_on);

/** @brief turn on colorization for output channel, non of the
 *         io primitives use this (such as io_putc, io_puts, etc),
 *         but other functions that have access to the internals
 *         do, such as the lisp interpreter.
 *  @param out       output channel to set pretty printing on
 *  @param pretty_on non zero for true (pretty printing on), 
 *                   zero for false**/
void io_pretty(io *out, int pretty_on);

/********************* translate or delete characters ************************/

/** @brief  create a new tr state for use in character translation
 *  @return a new tr state or NULL**/
tr_state *tr_new(void);

/** @brief destroy a tr state
 *  @param st tr state to delete**/
void tr_delete(tr_state *st);

/** @brief initialize a translation state 
 *  
 *  The initialization routine does the work of deciding what characters
 *  should be mapped to, or deleted, from one block to another. Valid mode
 *  characters include the following:
 *
 *      ''  (empty string) normal translation
 *      'x' normal translation
 *      'c' compliment set 1
 *      's' squeeze repeated character run
 *      'd' delete characters (s2 should be NULL)
 *      't' truncate set 1 to the size of set 2
 *
 *  @param  tr   tr state to initialize
 *  @param  mode translate mode
 *  @param  s1   character set 1
 *  @param  s2   character set 2
 *  @return int  negative on failure, zero on success **/
int tr_init(tr_state *tr, char *mode, uint8_t *s1, uint8_t *s2);

/** @brief translate a single character given a translation state
 *  @param  tr  translation state
 *  @param  c   character to translate
 *  @return int negative if the character should be deleted, greater or equal to
 *              zero is the new character **/
int tr_char(tr_state *tr, uint8_t c);

/** @brief translate a block of memory given an initialized tr state
 *  @param  tr     tr state
 *  @param  in     input buffer
 *  @param  out    output buffer
 *  @param  len    length of both the input and output buffer
 *  @return size_t number of characters written to the output buffer**/
size_t tr_block(tr_state *tr, uint8_t *in, uint8_t *out, size_t len);

/******************* functions for manipulating lisp cell ********************/

/* This module is used by the internals of the lisp interpreter
 * for manipulating lisp cells; for accessing them, creating new
 * cells and testing their type. */

cell *car(cell *x); /**< get car cell from cons**/
cell *cdr(cell *x); /**< get cdr cell from cons**/
int  cklen(cell *x, size_t expect); /**< get length of list**/
cell *cons(lisp *l, cell *x, cell *y); /**< create a new cons cell**/
cell *extend(lisp *l, cell *env, cell *sym, cell *val);
cell *findsym(lisp *l, char *name); /**< find a previously used symbol**/
cell *intern(lisp *l, char *name); /**< add a new symbol**/
intptr_t intval(cell *x); /**< cast lisp cell to integer**/
io*  ioval(cell *x);    /**< cast lisp cell to I/O stream**/
int  is_nil(cell *x);    /**< true if 'x' is equal to nil**/
int  is_int(cell *x);    /**< true if 'x' is a integer **/
int  is_floatval(cell *x);  /**< true if 'x' is a floating point number**/
int  is_cons(cell *x);   /**< true if 'x' is a cons cell**/
int  is_io(cell *x);     /**< true if 'x' is a I/O type**/
int  is_proc(cell *x);   /**< true if 'x' is a lambda procedure**/
int  is_fproc(cell *x);  /**< true if 'x' is a flambda procedure**/
int  is_str(cell *x);    /**< true if 'x' is a string**/
int  is_sym(cell *x);    /**< true if 'x' is a symbol**/
int  is_subr(cell *x);   /**< true if 'x' is a language primitive**/
int  is_asciiz(cell *x); /**< true if 'x' is a ASCII nul delimited string**/
int  is_arith(cell *x);  /**< true if 'x' is integer or float**/
int  is_in(cell *x);     /**< true if 'x' is an input I/O type*/
int  is_out(cell *x);    /**< true if 'x' is an output I/O type*/
int  is_hash(cell *x);   /**< true if 'x' is a hash table type*/
int  is_userdef(cell *x); /**< true if 'x' is a user defined type*/
int  is_usertype(cell *x, int type); /**< is a specific user defined type**/
int  is_func(cell *x);   /**< true if 'x' can be applied (is a function) */
cell *mkint(lisp *l, intptr_t d); /**< make a lisp cell from an integer**/
cell *mkfloat(lisp *l, lfloat f); /**< make a lisp cell from a float**/
cell *mkio(lisp *l, io *x); /**< make lisp cell from an I/O stream**/
cell *mksubr(lisp *l, subr p); /**< make a lisp cell from a primitive**/
cell *mkproc(lisp *l, cell *x, cell *y, cell *z); /**< make a lisp cell/proc**/
cell *mkfproc(lisp *l, cell *x, cell *y, cell *z); /**< make a lisp cell/fproc**/
cell *mkstr(lisp *l, char *s); /**< make lisp cell (string) from a string**/
/*cell *mksym(lisp *l, char *s); use intern instead to get a unique symbol*/
cell *mkhash(lisp *l, hashtable *h); /**< make lisp cell from hash**/
cell *mkuser(lisp *l, void *x, int type); /**< make a user defined type**/
subr subrval(cell *x); /**< cast a lisp cell to a primitive func ptr**/
cell *procargs(cell *x); /**< get args to a procedure**/
cell *proccode(cell *x); /**< get code from a proccode**/
cell *procenv(cell *x);  /**< get procedure environment**/
void setcar(cell *x, cell *y); /**< set cdr cell of a cons cell**/
void setcdr(cell *x, cell *y); /**< set car cell of a cons cell**/
char *strval(cell *x); /**< get string from a lisp cell**/
char *symval(cell *x); /**< get string (symbol) from a lisp cell**/
void *userval(cell *x); /**< get data from user defined type**/
lfloat floatval(cell *x); /**< get floating point val from lisp cell**/
hashtable *hashval(cell *x); /**< get hash table from a lisp cell**/
cell *mkerror(void); /**< return the error cell**/
cell *mknil(void);   /**< return the nil cell**/
cell *mktee(void);   /**< return the tee cell**/
cell *mkquote(void); /**< return the quote cell**/

/**@brief  return a new token representing a new type
 * @param  l lisp environment to put the new type in
 * @param  f function to call when freeing type, optional
 * @param  m function to call when marking type, optional
 * @param  e function to call when comparing two types, optional
 * @param  p function to call when printing type, optional
 * @return int return -1 if there are no more tokens to give or a positive
 *                number representing a user defined token*/
int newuserdef(lisp *l, ud_free f, ud_mark m, ud_equal e, ud_print p);  

/**@brief determines whether a string contains a number that
 *        can be converted with strtol.
 *        matches "(+|-)?(0[xX][0-9a-fA-F]+|0[0-7]*|[1-9][0-9]+)" 
 * @param  buf ascii delimited string to check if it is a number
 * @return 0 if it is not a number, non zero if it is**/
int is_number(const char *buf);

/**@brief  this is much like is_number, but for floating point numbers
 *         matches "[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?", and
 *         excludes valid conversions added in c99 (such as "inf" or
 *         "nan").
 * @param  buf ascii delimited string to check if it is a float
 * @return 0 if it is not a float, non zero if it is**/
int is_fnumber(const char *buf);

/*********** functions for implementing an interpreter environment ***********/

/* This module allows for reading, printing and evaluating of 
 * S-expressions. The reader can be used as a generic way to parser
 * S-expressions, which is part of the intention of the library. */

/** @brief A method for throwing an exception in the lisp interpreter,
 *         this will call exit() if internally a jmp_buf has not been
 *         set for this throw to return to. 
 *
 *  A positive number signals that the lisp interpreter can continue
 *  after processing this error, a negative number that it should exit
 *  to whatever called the interpreter. All of the lisp_* functions
 *  such as "lisp_eval" should internally set the jmp_buf to recover
 *  to.
 *
 *  The purpose of this function for the library user is so that they
 *  can call lisp_throw() from inside their user defined functions
 *  which can be added with lisp_add_subr().
 *
 *  @param l   a lisp environment to throw an exception in, if NULL
 *             this will call exit(ret) instead!
 *  @param ret a negative ret will halt the interpreter, a positive
 *             one signals an error occurred, but it can be recovered
 *             from**/
void lisp_throw(lisp *l, int ret);

/** @brief Formatted printing of lisp cells, colorized if color enabled on
 *         output stream, it can print out s-expression trees generated by
 *         by "lisp_repl()" or "lisp_read()". The output of s-expressions
 *         trees can be pretty printed, the "depth" parameter specifies
 *         the indentation level.
 *              
 *         The "fmt" format string acts like the "printf" format string,
 *
 *         '%'  print out a single escape char '%'
 *         '*'  print out "depth" many of the next character
 *         'c'  print out a single (c)haracter        
 *         's'  print out a (s)tring
 *         'S'  print out a (S)-expression
 *         
 *         Color/Terminal control options. At the moment they assume that
 *         your terminal can handle ANSI escape codes, which is nearly
 *         every single terminal.
 *
 *         't'  rest (t)erminal color to default
 *         'B'  (B)old text
 *         'v'  reverse (v)ideo
 *         'k'  blac(k)
 *         'r'  (r)ed
 *         'g'  (g)reen
 *         'y'  (y)ellow
 *         'b'  (b)lue
 *         'm'  (m)agenta
 *         'a'  cy(a)n
 *         'w'  (w)hite
 *
 *  @param  l      an initialized lisp environment, this can be NULL but
 *                 maximum depth will not be checked, nor will user defined
 *                 functions be printed out with their user defined printing
 *                 functions for that environment.
 *  @param  o      output I/O stream to print to
 *  @param  depth  indentation level of any pretty printing
 *  @param  fmt    format string
 *  @param  ...    any arguments
 *  @return int    >= 0 on success, less than 0 on failure**/
int printerf(lisp *l, io *o, unsigned depth, char *fmt, ...);

/** @brief  add a symbol to the list of all symbols
 *  @param  l    lisp environment to add cell to
 *  @param  ob   cell to add
 *  @return cell* NULL on failure, not NULL on success**/
cell *lisp_intern(lisp *l, cell *ob);

/** @brief  add a symbol-val pair and intern a lisp cell 
 *  @param  l    lisp environment to add pair to
 *  @param  sym  name of symbol
 *  @param  val  value to add
 *  @return NULL on failure, not NULL on success **/
cell *lisp_add_cell(lisp *l, char *sym, cell *val);

/** @brief  add a function primitive to a lisp environment. It will be
 *          referenced internally by the "name" string.
 *  @param  l     lisp environment to add primitive to
 *  @param  name  name to call the function primitive by
 *  @param  func  function primitive
 *  @return cell* pointer to extended environment if successful, NULL
 *                otherwise. You shouldn't do anything with pointer**/
cell *lisp_add_subr(lisp *l, char *name, subr func);

/** @brief  Initialize a lisp environment. By default it will read
 *          from stdin, print to stdout and log errors to stderr.
 *  @return lisp*    A fully initialized lisp environment or NULL**/
lisp *lisp_init(void);

/** @brief  read in a s-expression, it uses the lisp environment
 *          for error handling, garbage collection and finding
 *          duplicate symbols
 *  @param  l     an initialized lisp environment
 *  @param  i     I/O stream to read from
 *  @return cell* a parsed s-expression or NULL on failure**/
cell *lisp_read(lisp *l, io *i);

/** @brief  print out an s-expression
 *  @param  l    a lisp environment which contains the IO stream to print to
 *  @param  ob   cell to print
 *  @return int  0 >= on success, less than 0 on failure**/
int lisp_print(lisp *l, cell *ob);

/** @brief  evaluate a lisp expression
 *  @param  l     a initialized lisp environment to evaluate against
 *  @param  exp   an expression to evaluate
 *  @return cell* a lisp expression to print out, or NULL**/
cell *lisp_eval(lisp *l, cell *exp);

/** @brief  parse and evaluate a string, returning the result, it will
 *          however discard any input after the first evaluation.
 *
 *          For example:
 *          (+ 2 2)
 *          will be evaluated to an integer cell containing 4.
 *
 *          Whereas:
 *          (+ 2 2) (+ 3 3)
 *          The first expression will be evaluated, the second one "(+ 3 3)"
 *          will not be.
 *
 *          The same goes for atoms:
 *          2 "Hello World"
 *          Will return 2, "Hello World" will not even be evaluated.
 *
 *          To evaluate a list of expressions you could use either:
 *          (list (expr) (expr) ... (expr))
 *          Or use lisp_repl.
 *
 *  @param  l       lisp environment to evaluate in
 *  @param  evalme  string to evaluate
 *  @return cell*   result of evaluation or NULL on failure critical failure**/
cell *lisp_eval_string(lisp *l, char *evalme);

/** @brief  a simple Read-Evaluate-Print-Loop (REPL)
 *  @param  l      an initialized lisp environment
 *  @param  prompt a  prompt to print out, use the empty string for no prompt
 *  @param  editor_on if a line editor has been provided with
 *                    lisp_set_line_editor then this turns it on (on is non
 *                    zero).
 *  @return int    return non zero on error, negative on fatal error**/
int lisp_repl(lisp *l, char *prompt, int editor_on);

/** @brief destroy a lisp environment
 *  @param l       lisp environment to destroy**/
void lisp_destroy(lisp *l);

/** @brief set the input for a lisp environment
 *  @param  l  an initialized lisp environment
 *  @param  in the input port
 *  @return int zero on success, non zero otherwise**/
int lisp_set_input(lisp *l, io *in);

/** @brief set the output for a lisp environment
 *  @param  l   an initialized lisp environment
 *  @param  out the output port
 *  @return int zero on success, non zero otherwise**/
int lisp_set_output(lisp *l, io *out);

/** @brief set the output for error logging for a lisp environment
 *  @param l       an initialized lisp environment
 *  @param logging the output port for error logging
 *  @return int    zero on success, non zero otherwise**/
int lisp_set_logging(lisp *l, io *logging);

/** @brief  set the line editor to be used with lisp_repl, main_lisp
 *          and main_lisp_env both call lisp_repl behind the scenes,
 *          and will enable line editor functionality when reading from
 *          stdin and passed the right option. The editor_func accepts
 *          a string to print as the prompt and returns a line of text
 *          that has been read in.
 *  @param  l      an initialized lisp environment
 *  @param  ed     the line editor function**/
void lisp_set_line_editor(lisp *l, editor_func ed);

/** @brief  set the internal signal handling variable of a lisp environment,
 *          this is a way for a function such as a signal handler or another
 *          thread to halt the interpreter.
 *  @param  l   an initialized lisp environment
 *  @param  sig signal value, any non zero value halts the lisp environment**/
void lisp_set_signal(lisp *l, int sig);

/** @brief get the input channel in use in a lisp environment
 *  @param  l lisp environment to retrieve input channel from
 *  @return io* pointer to input channel or NULL on failure**/
io *lisp_get_input(lisp *l);

/** @brief get the output channel in use in a lisp environment
 *  @param  l lisp environment to retrieve output channel from
 *  @return io* pointer to output channel or NULL on failure**/
io *lisp_get_output(lisp *l);

/** @brief get the logging/error channel in use in a lisp environment
 *  @param  l lisp environment to retrieve error channel from
 *  @return io* pointer to error channel or NULL on failure**/
io *lisp_get_logging(lisp *l);

/************************ test environment ***********************************/

/** @brief  A full lisp interpreter environment in a function call. It will
 *          read from stdin by default and print to stdout, but that is
 *          dependent on the arguments given to it. See the man pages (or
 *          source) for a full list of commands.
 *  @param  argc argument count
 *  @param  argv arguments to pass into interpreter
 *  @return int  return value of interpreter, 0 is normal termination**/
int main_lisp(int argc, char **argv);

/** @brief  This is like the main_lisp function, however you can pass in
 *          an initialized lisp environment, and before this 
 *          add functionality to the environment you want
 *  @param  l     an initialized lisp environment
 *  @param  argc  argument count
 *  @param  argv  arguments to pass to interpreter
 *  @return int **/
int main_lisp_env(lisp *l, int argc, char **argv);

/*********************** some handy macros ***********************************/

/**@brief A wrapper around vstrcatsep so that the user does not have to
 *        terminate the sequence with a null. Like vstrcatsep is will 
 *        concatenate a variable amount of strings with a separator
 *        inbetween them.
 * @param  SEP    a string to insert between the other strings
 * @param  STR    the first string to concatenate
 * @param  ...    a list of strings, no NULL sentinel is needed
 * @return char*  a NUL terminated string that can be freed**/
#define VSTRCATSEP(SEP, STR, ...) vstrcatsep((SEP), (STR), __VA_ARGS__, (char*)0)

/**@brief  A wrapper around vstrcatsep, with no separator and no need
 *         for a sentinel. This concatenates a list of strings together.
 * @param  STR    the first string to concatenate
 * @param  ...    a list of strings, no NULL sentinel is needed
 * @return char* a NUL terminated string that can be freed**/
#define VSTRCAT(STR, ...) vstrcatsep("",    (STR), __VA_ARGS__, (char*)0)

/**@brief  A wrapper around vstrcatsep that only concatenates two string
 *         together.
 * @param  S1    the first string
 * @param  S2    the second string
 * @return char* a NUL terminated string that can be freed**/
#define CONCATENATE(S1, S2) vstrcatsep("",    (S1),  (S2), (char*)0)

/**@brief  A wrapper around fprintf that will print out a nicely formatted
 *         error message with line number, current function and line.
 * @param  FMT fprintf format string
 * @param  ... Arguments to the fprintf format string
 * @return int fprintf return code**/
#define PRINT_ERROR(FMT, ...) fprintf(stderr,\
                                    "(error " FMT " '%s '%s %d)\n",\
                                     __VA_ARGS__, __FILE__, __func__, __LINE__)

/**@brief Print out an error message and throw a lisp exception.
 * @param LISP The lisp environment to throw the exception in
 * @param RET  Internal lisp exception return code
 * @param FMT  Format string printing error messages
 * @param ...  Arguments for format string**/
#define FAILPRINTER(LISP, RET, FMT, ...) do{\
        printerf((LISP), lisp_get_logging((LISP)), 0,\
                        "(error '%s " FMT " \"%s\" %d)\n",\
                       __func__, __VA_ARGS__, __FILE__, (intptr_t)__LINE__);\
        lisp_throw((LISP), RET);\
        } while(0)

/**@brief Print out an error message and throw a lisp exception that
 *        can be recovered from.
 * @param LISP The lisp environment to throw the exception in
 * @param FMT  Format string printing error messages
 * @param ...  Arguments for format string**/
#define RECOVER(LISP, FMT, ...) FAILPRINTER(LISP,  1, FMT, __VA_ARGS__)

/**@brief Print out an error message and throw a lisp exception that
 *        the interpreter cannot recover from, it will return.
 * @param LISP The lisp environment to throw the exception in
 * @param FMT  Format string printing error messages
 * @param ...  Arguments for format string**/
#define HALT(LISP, FMT, ...) FAILPRINTER(LISP, -1, FMT, __VA_ARGS__)

/**@brief A wrapper around pfatal that with file and line filled out
 *        for the error message to print.
 * @param MSG Message to print out**/
#define FATAL(MSG) pfatal((MSG), __FILE__, __LINE__)

/**@brief Stringify X, turn X into a string.
 * X      Thing to turn into a string  **/
#define STRINGIFY(X) #X

/**@brief Stringify with macro expansion
 * X      Thing to turn into a string with macro expansion  **/
#define XSTRINGIFY(X) STRINGIFY(X)

/**@brief Like assert() but will not be disabled by NDEBUG. It would be
 *        nice if this would print a stack trace but there is no portable
 *        way of doing this without make the C code look ugly with macro
 *        magic.
 * X      Test to perform**/
#define ASSERT(X) do { if(!(X)) FATAL("assertion failed: " # X ); } while(0)

/**@brief Swap two values, with a specified type. For example to swap
 *        two integers values 'a' and 'b': SWAP(a, b, int);
 * @param X    variable X
 * @param Y    variable Y
 * @param TYPE type of variables**/
#define SWAP(X, Y, TYPE) do { TYPE SWAP = X; X = Y; Y = SWAP; } while(0);

#ifndef ABS
#define ABS(X) ((X) < 0 ? -(X) : (X)) /**< absolute value*/
#endif

#ifndef MAX
#define MAX(X, Y)    ((X)>(Y)?(X):(Y)) /**< largest of two values*/
#endif

#ifndef MIN
#define MIN(X, Y)    ((X)>(Y)?(Y):(X)) /**< smallest of two values*/
#endif

/* The following macros are helper macros for lisp list access */
#define CAAR(X)   car(car((X)))
#define CADR(X)   car(cdr((X)))
#define CDAR(X)   cdr(car((X)))
#define CDDR(X)   cdr(cdr((X)))
#define CADAR(X)  car(cdr(car((X))))
#define CADDR(X)  car(cdr(cdr((X))))
#define CADDDR(X) car(cdr(cdr(cdr((X)))))

#ifdef __cplusplus
}
#endif
#endif /* LIBLISP_H */

