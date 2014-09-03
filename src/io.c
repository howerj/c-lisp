/**
 *  @file           io.c
 *  @brief          I/O redirection and wrappers
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 **/

#include "type.h"
#include "io.h"

/**** I/O functions **********************************************************/

/**
 *  @brief          wrapper around putc; redirect output to a file or string
 *  @param          c output this character
 *  @param          o output stream
 *  @param          e error output stream
 *  @return         EOF on failure, character to output on success
 **/
int io_putc(char c, io * o, io * e)
{
        NULLCHK(o, e);
        NULLCHK(o->ptr.file, e);

        if (file_out == o->type) {
                return fputc(c, o->ptr.file);
        } else if (string_out == o->type) {
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
 *  @param          i input stream
 *  @param          e error output stream
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

        if (file_in == i->type) {
                return fgetc(i->ptr.file);
        } else if (string_in == i->type) {
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
 *  @param          i input stream to put character back into
 *  @param          e error output stream
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
 *  @param          o output stream to print to
 *  @param          e error output stream
 *  @return         negative number if operation failed, otherwise the
 *                  total number of characters written
 **/
int io_printd(cell_t d, io * o, io * e)
{
  /**@todo rewrite so it does not use sprintf/fprintf**/
        NULLCHK(o, e);
        if (file_out == o->type) {
                return fprintf(o->ptr.file, "%d", d);
        } else if (string_out == o->type) {
                return sprintf(o->ptr.string + o->position, "%d", d);
        } else {
                /*programmer error; some kind of error reporting would be nice */
                exit(EXIT_FAILURE);
        }
        return -1;              /* returns negative like printf would on failure */
}

/**
 *  @brief          wrapper to print out a pointer; this should be rewritten
 *                  to avoid using fprintf and sprintf 
 *  @param          p pointer to print out
 *  @param          o output stream to print to
 *  @param          e error output stream
 *  @return         negative number if operation failed, otherwise the
 *                  total number of characters written
 **/
int io_printp(void *p, io * o, io * e)
{
  /**@todo rewrite so it does not use sprintf/fprintf**/
        NULLCHK(o, e);
        if (file_out == o->type) {
                return fprintf(o->ptr.file, "%p", p);
        } else if (string_out == o->type) {
                return sprintf(o->ptr.string + o->position, "%p", p);
        } else {
                /*programmer error; some kind of error reporting would be nice */
                exit(EXIT_FAILURE);
        }
        return -1;              /* returns negative like printf would on failure */
}

/**
 *  @brief          wrapper to print out a string, *does not append newline*
 *  @param          s string to output
 *  @param          o output stream to print to
 *  @param          e error output stream
 *  @return         EOF on failure, number of characters written on success
 *                  
 **/
int io_puts(const char *s, io * o, io * e)
{
  /**@warning count can go negative when is should not!**/
        int count = 0;
        int c;
        NULLCHK(o, e);
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
 *  @param          cfile   C file the error occurred in (__FILE__)
 *  @param          linenum line of C file error occurred (__LINE__)
 *  @param          e       error output stream to print to
 *  @return         void
 *                  
 **/
void doreport(const char *s, char *cfile, unsigned int linenum, io * e)
{
        io n_e = { file_out, {NULL}, 0, 0, '\0', false };
        bool critical_failure_f = false;
        n_e.ptr.file = stderr;

        if ((NULL == e) || (NULL == e->ptr.file)
            || ((file_out != e->type) && (string_out != e->type))
            ) {
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
