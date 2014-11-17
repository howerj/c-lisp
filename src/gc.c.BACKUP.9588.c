/**
 *  @file           gc.c
 *  @brief          A precise garbage collector
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 *  @details
 *
 *  Garabage collection and error handling on
 *  Out-Of-Memory errors should go here.
 */

#include "gc.h"
#include "mem.h"

struct heap {
        expr x;
        struct heap *next;
};

static struct heap heaplist = { NULL, NULL };

static struct heap *heaphead = &heaplist;

static void gcinner(expr x);

/*** interface functions *****************************************************/

/**
 *  @brief          wrapper around malloc for garbage collection
 *  @return         pointer to newly allocated storage on sucess, exits
 *                  program on failure!
 **/
expr gc_malloc(void)
{
        void *v;
        v = mem_malloc(sizeof(struct sexpr_t));
        return v;
}

/**
 *  @brief          wrapper around calloc for garbage collection
 *  @return         pointer to newly allocated storage on sucess, which
 *                  is zeroed, exits program on failure!
 **/
expr gc_calloc(void)
{
        expr v;
        struct heap *nextheap;
        v = mem_calloc(1, sizeof(struct sexpr_t));
        nextheap = mem_calloc(1, sizeof(struct heap));
        nextheap->x = v;
        heaphead->next = nextheap;
        heaphead = nextheap;
        return v;
}

/**
 *  @brief          Given a root structure, mark all accessible
 *                  objects in the tree so they do not get garbage
 *                  collected
 *  @param          root root tree to mark
 *  @return         false == root was not marked, and now is
 **/
int gc_mark(expr root)
{
}

/**
 *  @brief          Sweep all unmarked objects.
 *  @return         void
 **/
void gc_sweep(void)
{
}

/*****************************************************************************/

/**
 *  @brief          Frees expressions
 *  @param          x     expression to print
 *  @return         void
 **/
static void gcinner(expr x)
{
<<<<<<< HEAD
=======
        if (NULL == x)
                return;

        switch (x->type) {
        case S_TEE:
        case S_NIL:
        case S_INTEGER:
        case S_PRIMITIVE:
                mem_free(x);
                break;
        case S_PROC:
                mem_free(x->data.list);
                mem_free(x);
                break;
        case S_LIST:
                mem_free(x->data.list);
                mem_free(x);
                return;
        case S_SYMBOL:
                mem_free(x->data.symbol);
                mem_free(x);
                return;
        case S_STRING:
                mem_free(x->data.string);
                mem_free(x);
                return;
        case S_ERROR:
                /** @todo implement error support **/
                mem_free(x);
                return;
        case S_FILE:
               /** @todo implement file support **/
                IO_REPORT("UNIMPLEMENTED (TODO)");
                break;
        case S_QUOTE:
                IO_REPORT("UNIMPLEMENTED (TODO)");
                break;
        case S_LAST_TYPE:
        default:               /* should never get here */
                IO_REPORT("free: not a known 'free-able' type");
                exit(EXIT_FAILURE);
                return;
        }
>>>>>>> master
}

/*****************************************************************************/
