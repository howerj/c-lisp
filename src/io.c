/**
 *  @file           io.c
 *  @brief          I/O redirection and wrappers
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 *
 *  @todo Implement set_error_stream instead of passing the error
 *        constantly to functions like io_puts
 *  @todo Use stdarg.h where appropriate
 *  @todo Error checking on return values.
 *  @todo printf and scanf equivalent that deals with fixed width
 *        types and avoids floating point conversions.
 *
 *  This library allows redirection of input and output to
 *  various different sources. It also would allow me to add in
 *  a new arbitrary source later on, for example reading and
 *  writing to and from sockets.
 *
 *  It is possible to implement something similar using 'setbuf' and
 *  family from <stdio.h> and perhaps more cleanly and efficiently as
 *  well. However it would not *quite* replicate what I want and such
 *  functionality would have to be wrapped up anyway.
 *
 **/

#include "io.h"
#include <assert.h>  
#include <string.h>  
#include <stdlib.h>  
#include <stdbool.h> 

#define NULLCHK(X,E)  if(NULL == (X))\
                      { REPORT("null dereference",(E)); exit(EXIT_FAILURE);}

/**I/O abstraction structure**/
struct io {
        union {
                FILE *file;
                char *string;
        } ptr; 

        size_t position;        /* position in string */
        size_t max;             /* max string length, if known */

        enum iotype {
                IO_INVALID_E,   /* error on incorrectly set up I/O */
                IO_FILE_IN_E,   /* read from file */
                IO_FILE_OUT_E,  /* write to file */
                IO_STRING_IN_E, /* read from a string */
                IO_STRING_OUT_E /* write to a string, if you want */
        } type;

        bool ungetc;            /* true if we have ungetc'ed a character */
        char c;                 /* character store for io_ungetc() */
};

static int io_itoa(int32_t d, char *s); /* I *may* want to export this later */

static io error_stream = {{NULL}, 0, 0, IO_FILE_OUT_E, false, '\0'};

/**** I/O functions **********************************************************/

/**
 *  @brief          Set input wrapper to read from a string
 *  @param          i           input stream, Do not pass NULL
 *  @param          s           string to read from, Do not pass NULL
 *  @return         void
 **/
void io_string_in(io *i, char *s){
        assert((NULL != i) && (NULL != s));
        memset(i, 0, sizeof(*i));
        i->type         = IO_STRING_IN_E;
        i->ptr.string   = s;
        i->max          = strlen(s);
        return;
}

/**
 *  @brief          Set output stream to point to a string
 *  @param          o           output stream, Do not pass NULL
 *  @param          s           string to write to, Do not pass NULL
 *  @return         void
 **/
void io_string_out(io *o, char *s){
        assert((NULL != o) && (NULL != s));
        memset(o, 0, sizeof(*o));
        o->type         = IO_STRING_OUT_E;
        o->ptr.string   = s;
        o->max          = strlen(s);
        return;
}

/**
 *  @brief          Attempts to open up a file called file_name for
 *                  reading, setting the input stream wrapper to read
 *                  from it.
 *  @param          i           input stream, Do not pass NULL
 *  @param          file_name   File to open and read from, Do not pass NULL
 *  @return         FILE*       Initialized FILE*, or NULL on failure
 **/
FILE *io_filename_in(io *i, char *file_name){
        assert((NULL != i) && (NULL != file_name));
        memset(i, 0, sizeof(*i));
        i->type         = IO_FILE_IN_E;
        if(NULL == (i->ptr.file = fopen(file_name, "rb")))
                return NULL;
        return i->ptr.file;
}

/**
 *  @brief          Attempt to open file_name for writing, setting the
 *                  output wrapper to use it
 *  @param          o           output stream, Do not pass NULL
 *  @param          file_name   file to open, Do not pass NULL
 *  @return         FILE*       Initialized file pointer, or NULL on failure
 **/
FILE *io_filename_out(io *o, char *file_name){
        assert((NULL != o)&&(NULL != file_name));
        memset(o, 0, sizeof(*o));
        o->type         = IO_FILE_OUT_E;
        if(NULL == (o->ptr.file = fopen(file_name, "wb")))
                return NULL;
        return o->ptr.file;
}

/**
 *  @brief          Set input stream wrapper to point to a FILE*
 *  @param          i           input stream, Do not pass NULL
 *  @param          file        file to read from, Do not pass NULL
 *  @return         void
 **/
void io_file_in(io *i, FILE* file){
        assert((NULL != i)&&(NULL != file));
        memset(i, 0, sizeof(*i));
        i->type         = IO_FILE_IN_E;
        i->ptr.file     = file;
        return;
}

/**
 *  @brief          Set an output stream wrapper to use a FILE*
 *  @param          o           output stream, Do not pass NULL
 *  @param          file        FILE* to write to, Do not pass NULL
 *  @return         void
 **/
void io_file_out(io *o, FILE* file){
        assert((NULL != o));
        assert((NULL != file));
        memset(o, 0, sizeof(*o));
        o->type         = IO_FILE_OUT_E;
        o->ptr.file     = file;
        return;
}

/**
 *  @brief          Flush and close an input or output stream, this *will not* close
 *                  stdin, stdout or stderr, but it will flush them and invalidate
 *                  the IO wrapper struct passed to it.
 *  @param          ioc         Input or output stream to close, Do not pass NULL
 *  @return         void
 **/
void io_file_close(io *ioc){
        assert(NULL != ioc);

        if((IO_FILE_IN_E == ioc->type) || (IO_FILE_OUT_E == ioc->type)){
                if(NULL != ioc->ptr.file){
                        fflush(ioc->ptr.file);
                        if((ioc->ptr.file != stdin) && (ioc->ptr.file != stdout) && (ioc->ptr.file != stdin))
                                fclose(ioc->ptr.file);
                        ioc->ptr.file = NULL;
                }
        }
        return;
}

/**
 *  @brief          Return the size of the 'io' struct, this is an incomplete
 *                  type to the outside world.
 *  @return         size_t size of io struct which is hidden from the outside
 **/
size_t io_sizeof_io(void){
        return sizeof(io);
}

/**
 *  @brief          wrapper around putc; redirect output to a file or string
 *  @param          c   output this character
 *  @param          o   output stream, Do not pass NULL
 *  @param          e   error output stream, Do not pass NULL
 *  @return         EOF on failure, character to output on success
 **/
int io_putc(char c, io * o, io * e)
{
        NULLCHK(o, e);
        NULLCHK(o->ptr.file, e);

        if (IO_FILE_OUT_E == o->type) {
                return fputc(c, o->ptr.file);
        } else if (IO_STRING_OUT_E == o->type) {
                if (o->position < o->max) {
                        o->ptr.string[o->position++] = c;
                        return c;
                } else {
                        return EOF;
                }
        } else {
                /*programmer error; some kind of error reporting would be nice */
                exit(EXIT_FAILURE);
        }
        return EOF;
}

/**
 *  @brief          wrapper around io_putc; get input from file or string
 *  @param          i input stream, Do not pass NULL
 *  @param          e error output stream, Do not pass NULL
 *  @return         EOF on failure, character input on success
 **/
int io_getc(io * i, io * e)
{
        NULLCHK(i, e);
        NULLCHK(i->ptr.file, e);

        if (true == i->ungetc) {
                i->ungetc = false;
                return i->c;
        }

        if (IO_FILE_IN_E == i->type) {
                return fgetc(i->ptr.file);
        } else if (IO_STRING_IN_E == i->type) {
                return (i->ptr.string[i->position]) ? i->ptr.string[i->position++] : EOF;
        } else {
                /*programmer error; some kind of error reporting would be nice */
                exit(EXIT_FAILURE);
        }
        return EOF;
}

/**
 *  @brief          wrapper around ungetc; unget from to file or string
 *  @param          c character to put back
 *  @param          i input stream to put character back into, Do not pass NULL
 *  @param          e error output stream, Do not pass NULL
 *  @return         EOF if failed, character we put back if succeeded.
 **/
int io_ungetc(char c, io * i, io * e)
{
        NULLCHK(i, e);
        NULLCHK(i->ptr.file, e);
        if (true == i->ungetc) {
                return EOF;
        }
        i->c = c;
        i->ungetc = true;
        return c;
}

/**
 *  @brief          wrapper to print out a number; this should be rewritten
 *                  to avoid using fprintf and sprintf 
 *  @param          d integer to print out
 *  @param          o output stream to print to, Do not pass NULL
 *  @param          e error output stream, Do not pass NULL
 *  @return         negative number if operation failed, otherwise the
 *                  total number of characters written
 **/
int io_printd(int32_t d, io * o, io * e)
{
        char dstr[16];
        NULLCHK(o, e);
        io_itoa(d, dstr);
        return io_puts(dstr, o, e);
}

/**
 *  @brief          wrapper to print out a string, *does not append newline*
 *  @param          s string to output, you *CAN* pass NULL
 *  @param          o output stream to print to, Do not pass NULL
 *  @param          e error output stream, Do not pass NULL
 *  @return         EOF on failure, number of characters written on success
 *                  
 **/
int io_puts(const char *s, io * o, io * e)
{
  /**@warning count can go negative when is should not!**/
        int count = 0;
        int c;
        NULLCHK(o, e);
        if(NULL == s)
                return;
        while ((c = *(s + (count++))))
                if (EOF == io_putc((char)c, o, e))
                        return EOF;
        return count;
}

/**
 *  @brief          A clunky function that should be rewritten for
 *                  error reporting. It is not used directly but instead
 *                  wrapped in a macro
 *  @param          s       error message to print
 *  @param          cfile   C file the error occurred in (__FILE__), Do not pass NULL
 *  @param          linenum line of C file error occurred (__LINE__)
 *  @param          e       error output stream to print to
 *  @return         void
 *                  
 **/
void io_doreport(const char *s, char *cfile, unsigned int linenum, io * e)
{
        io n_e = {{NULL}, 0, 0, IO_FILE_OUT_E, false, '\0'};
        bool critical_failure_f = false;
        n_e.ptr.file = stderr;

        if ((NULL == e) || (NULL == e->ptr.file) || ((IO_FILE_OUT_E != e->type) && (IO_STRING_OUT_E != e->type))) {
                e = &n_e;
                critical_failure_f = true;
        }

        io_puts("(error \"", e, e);
        io_puts(s, e, e);
        io_puts("\" \"", e, e);
        io_puts(cfile, e, e);
        io_puts("\" ", e, e);
        io_printd(linenum, e, e);
        io_puts(")\n", e, e);

        if (true == critical_failure_f) {
                io_puts("(error \"critical failure\")\n", e, e);
                exit(EXIT_FAILURE);
        }
        return;
}

/**** Internal functions *****************************************************/

/**
 *  @brief          Convert and integer to a string, base-10 only, the
 *                  casts are there for splint -weak checking.
 *  @param          d   integer to convert
 *  @param          s   string containing the integer, Do not pass NULL
 *  @return         int size of the converted string
 **/
static int io_itoa(int32_t d, char *s){
        int32_t sign, len;
        uint32_t v, i;
        char tb[sizeof(int32_t)*3+2]; /* maximum bytes num of new string */
        char *tbp = tb;
        assert(NULL != s);

        sign = (int32_t)(d < 0 ? -1 : 0);   
        v = (uint32_t)(sign == 0 ? d : -d);

        do{
                i = v % 10;
                v /= 10;
                *tbp++ = (char) i + ((i < 10) ? '0' : 'a' - 10);
        } while(v);

        len = tbp - tb;
        if(-1 == sign){
                *s++ = '-';
                len++;
        }

        while(tbp > tb)
                *s++ = *--tbp;
        *s = '\0';

        return (int)len;
}

