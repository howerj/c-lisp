/** @file       liblisp_diff.c
 *  @brief      diff util module
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com 
 *  @bug        This is very buggy!**/

#include "diff.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <liblisp.h>

static cell *make_diff_inner(lisp *l, diff *d, char *x[], char *y[], size_t i, size_t j, cell *pp, cell *mm) {
        if (i > 0 && j > 0 && !strcmp(x[i-1], y[j-1])) {
                return cons(l, 
                        cons(l, mk_str(l, lstrdup(x[i-1])), gsym_nil()), 
                        make_diff_inner(l, d, x, y, i-1, j-1, pp, mm));
        } else if (j > 0 && (i == 0 || d->c[(i*(d->n))+(j-1)] >= d->c[((i-1)*(d->n))+j])) {
                return cons(l, 
                        cons(l, pp, 
                                cons(l, mk_str(l, lstrdup(y[j-1])), gsym_nil())), 
                        make_diff_inner(l, d, x, y, i, j-1, pp, mm)); 
        } else if(i > 0 && (j == 0 || d->c[(i*(d->n))+(j-1)] < d->c[((i-1)*(d->n))+j])) {
                return cons(l,
                        cons(l, mm,
                                cons(l, mk_str(l, lstrdup(x[i-1])), gsym_nil())),
                        make_diff_inner(l, d, x, y, i-1, j, pp, mm));
        }
        return gsym_nil();
}

static cell *make_diff(lisp *l, diff *d, char **x, char **y) { assert(l && d && x && y);
        cell *pp = intern(l, "+"), *mm = intern(l, "-");
        return make_diff_inner(l, d, x, y, d->m, d->n, pp, mm);
}

static cell* subr_diff(lisp *l, cell *args) {
        size_t i;
        cell *a, *b, *tmp, *ret = NULL;
        char **aa, **bb;
        diff *d = NULL;
        a = car(args);
        b = CADR(args);
        if(!(aa = calloc(get_length(a)+2, sizeof(*aa)))) /**@bug +2??*/
                HALT(l, "\"%s\"", "out of memory");
        if(!(bb = calloc(get_length(b)+2, sizeof(*bb)))) /**@bug +2??*/
                HALT(l, "\"%s\"", "out of memory");
        for(tmp = a, i = 0; !is_nil(tmp); tmp = cdr(tmp), i++)
                if(!is_asciiz(car(tmp)))
                        goto cleanup;
                else
                        aa[i] = get_str(car(tmp));
        for(tmp = b, i = 0; !is_nil(tmp); tmp = cdr(tmp), i++)
                if(!is_asciiz(car(tmp)))
                        goto cleanup;
                else
                        bb[i] = get_str(car(tmp));
        if(!(d = lcs(aa, get_length(a), bb, get_length(b))))
                HALT(l, "\"%s\"", "out of memory");
        ret = make_diff(l, d, aa, bb);
cleanup: 
        free(aa);
        free(bb);
        if(d)
                free(d->c);
        free(d);
        if(ret)
                return ret;
        RECOVER(l, "\"expected two lists of strings\" '%S", args);
        return gsym_error();
}

static int initialize(void) {
        assert(lglobal);
        if(!(lisp_add_subr(lglobal, "diff", subr_diff, "c c", "print the diff of two lists of strings"))) goto fail;
        lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: diff loaded\n");
        return 0;
fail:   lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: diff load failure\n");
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__((constructor));
static void destruct(void)  __attribute__((destructor));
static void construct(void) { initialize(); }
static void destruct(void)  { }
#elif _WIN32
#include <windows.h>
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
        UNUSED(hinstDLL); UNUSED(lpvReserved);
        switch (fdwReason) {
            case DLL_PROCESS_ATTACH: initialize(); break;
            case DLL_PROCESS_DETACH: break;
            case DLL_THREAD_ATTACH:  break;
            case DLL_THREAD_DETACH:  break;
	    default: break;
        }
        return TRUE;
}
#endif

