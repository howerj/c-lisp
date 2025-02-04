/** @file       util.c
 *  @brief      utility functions that are used within the liblisp project, but
 *              would also be of use throughout the project.
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com**/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void pfatal(const char *msg, const char *file, const char *func, long line) {
	assert(msg && file);
	if (errno)
		fprintf(stderr, "(error \"%s\" \"%s\" \"%s\" %ld)\n", msg, file, func, line);
	else
		fprintf(stderr, "(error \"%s\" \"%s\" \"%s\" \"%s\" %ld)\n", msg, strerror(errno),file, func, line);
	abort();
}

char *lstrdup(const char *s) {
	assert(s);
	char *str = NULL;
	if (!(str = malloc(strlen(s) + 1)))
		return NULL;
	strcpy(str, s);
	return str;
}

char *lstrdup_or_abort(const char *s) {
	char *r = lstrdup(s);
	if (!r)
		FATAL("string duplication failed");
	return r;
}

static int matcher(char *pat, char *str, size_t depth, jmp_buf * bf) {
	if (!depth)
		longjmp(*bf, -1);
	if (!str)
		return 0;
 again:
        switch (*pat) {
	case '\0':
		return !*str;
	case '*':
		return matcher(pat + 1, str, depth - 1, bf) || (*str && matcher(pat, str + 1, depth - 1, bf));
	case '.':
		if (!*str)
			return 0;
		pat++;
		str++;
		goto again;
	case '\\':
		if (!*(pat + 1))
			return -1;
		if (!*str)
			return 0;
		pat++;		/*fall through */
	default:
		if (*pat != *str)
			return 0;
		pat++;
		str++;
		goto again;
	}
}

int match(char *pat, char *str) {
	assert(pat && str);
	jmp_buf bf;
	if (setjmp(bf))
		return -1;
	return matcher(pat, str, LARGE_DEFAULT_LEN, &bf);
}

uint32_t djb2(const char *s, size_t len) {
	assert(s);
	uint32_t h = 5381;	/*magic number this hash uses, it just is */
	size_t i = 0;
	for (i = 0; i < len; s++, i++)
		h = ((h << 5) + h) + (*s);
	return h;
}

char *getadelim(FILE * in, int delim) {
	assert(in);
	io_t io_in;
	memset(&io_in, 0, sizeof(io_in));
	io_in.p.file = in;
	io_in.type = IO_FIN;
	return io_getdelim(&io_in, delim);
}

char *getaline(FILE * in) {
	assert(in);
	return getadelim(in, '\n');
}

char *lstrcatend(char *dest, const char *src) {
	assert(dest && src);
	size_t sz = strlen(dest);
	strcpy(dest + sz, src);
	return dest + sz + strlen(src);
}

char *vstrcatsep(const char *separator, const char *first, ...) {
	size_t len = 0, seplen = 0, num = 0;
	char *retbuf = NULL, *va = NULL, *p = NULL;
	va_list argp1, argp2;

	if (!separator || !first)
		return NULL;
	len = strlen(first);
	seplen = strlen(separator);

	va_start(argp1, first);
	va_copy(argp2, argp1);
	while ((va = va_arg(argp1, char *)))
		 num++, len += strlen(va);
	va_end(argp1);

	len += (seplen * num);
	if (!(retbuf = malloc(len + 1)))
		return NULL;
	retbuf[0] = '\0';
	p = lstrcatend(retbuf, first);
	va_start(argp2, first);
	while ((va = va_arg(argp2, char *)))
		 p = lstrcatend(p, separator), p = lstrcatend(p, va);
	va_end(argp2);
	return retbuf;
}

int unbalanced(const char *sexpr, char lpar, char rpar) {
	assert(sexpr);
	int bal = 0, c = 0;
	while ((c = *sexpr++))
		if (c == lpar)
			bal++;
		else if (c == rpar)
			bal--;
		else if (c == '"') {
			while ((c = *sexpr++)) {
				if (c == '\\' && '"' == *sexpr)
					sexpr++;
				else if (c == '"')
					break;
				else
					continue;
			}
			if (!c)
				return bal;
		} else
			continue;
	return bal;
}

int is_number(const char *buf) {
	assert(buf);
	char conv[] = "0123456789abcdefABCDEF";
	if (!buf[0])
		return 0;
	if (buf[0] == '-' || buf[0] == '+')
		buf++;
	if (!buf[0])
		return 0;
	if (buf[0] == '0') {	/*shorten the conv table depending on numbers base */
		if (buf[1] == 'x' || buf[1] == 'X')
			conv[22] = '\0', buf += 2;
		else
			conv[8] = '\0';
	} else {
		conv[10] = '\0';
	}
	if (!buf[0])
		return 0;
	return buf[strspn(buf, conv)] == '\0';
}

int is_fnumber(const char *buf) {
	assert(buf);
	size_t i = 0;
	char conv[] = "0123456789";
	if (!buf[0])
		return 0;
	if (buf[0] == '-' || buf[0] == '+')
		buf++;
	if (!buf[0])
		return 0;
	i = strspn(buf, conv);
	if (buf[i] == '\0')
		return 1;
	if (buf[i] == 'e')
		goto expon;	/*got check for valid exponentiation */
	if (buf[i] != '.')
		return 0;
	buf = buf + i + 1;
	i = strspn(buf, conv);
	if (buf[i] == '\0')
		return 1;
	if (buf[i] != 'e' && buf[i] != 'E')
		return 0;
 expon:
        buf = buf + i + 1;
	if (buf[0] == '-' || buf[0] == '+')
		buf++;
	if (!buf[0])
		return 0;
	i = strspn(buf, conv);
	if (buf[i] == '\0')
		return 1;
	return 0;
}

char *breverse(char *s, size_t len) {
	size_t i = 0;
	do {
		const char c = s[i];
		s[i] = s[len - i];
		s[len - i] = c;
	} while (i++ < (len / 2));
	return s;
}

static const char conv[] = "0123456789abcdefghijklmnopqrstuvwxzy";
char *dtostr(intptr_t d, unsigned base) {
	assert(base > 1 && base < 37);
	intptr_t i = 0, neg = d;
	uintptr_t x = d;
	char s[64 + 2] = "";
	if (x > INTPTR_MAX)
		x = -x;
	do {
		s[i++] = conv[x % base];
	} while ((x /= base) > 0);
	if (neg < 0)
		s[i++] = '-';
	return lstrdup(breverse(s, i - 1));
}

char *utostr(uintptr_t u, unsigned base) {
	assert(base > 1 && base < 37);
	uintptr_t i = 0;
	char s[64 + 1] = "";
	do {
		s[i++] = conv[u % base];
	} while ((u /= base));
	return lstrdup(breverse(s, i - 1));
}

/*tr functionality*/

static int tr_getnext(uint8_t ** s) {
	uint8_t seq[5] = "0000";
	if ((*s)[0] == '\0') {
	} else if ((*s)[0] == '\\') {
		switch ((*s)[1]) {
		case 'a':
			(*s) += 2;
			return '\a';
		case 'b':
			(*s) += 2;
			return '\b';
		case 'f':
			(*s) += 2;
			return '\f';
		case 'n':
			(*s) += 2;
			return '\n';
		case 'r':
			(*s) += 2;
			return '\r';
		case 't':
			(*s) += 2;
			return '\t';
		case 'v':
			(*s) += 2;
			return '\v';
		case '-':
			(*s) += 2;
			return '-';
		case '\\':
			(*s) += 2;
			return '\\';
		case '\0':
			return -1;
		default:
			break;
		}
		if (strspn((char *)(*s) + 1, "01234567") > 2) {
			int r = strtol((char *)memcpy(seq + 1, (*s) + 1, sizeof(seq) - 1) - 1, NULL, 8) & 0377;
			*s += 4;
			return r;
		}
	} else {
		return (*s)++, (*s - 1)[0];
	}
	return -1;
}

int tr_init(tr_state_t * tr, char *mode, uint8_t * s1, uint8_t * s2) {
	unsigned i = 0;
	int c = 0, d = 0, cp = 0, dp = 0;
	assert(tr && mode && s1);	/*s2 is optional */
	memset(tr, 0, sizeof(*tr));
	while ((c = mode[i++]))
		switch (c) {
		case 'x':
			break;
		case 'c':
			tr->compliment_seq = 1;
			break;
		case 's':
			tr->squeeze_seq = 1;
			break;
		case 'd':
			tr->delete_seq = 1;
			break;
		case 't':
			tr->truncate_seq = 1;
			break;
		default:
			return TR_EINVAL;
		}

	for (i = 0; i < 256; i++)
		tr->set_tr[i] = i;

	if (tr->delete_seq) {
		if (s2 || tr->truncate_seq)	/*set 2 should be NULL in delete mode */
			return TR_DELMODE;
		while ((dp = tr_getnext(&s1)) > 0)
			tr->set_del[dp] = 1;
		return TR_OK;
	}

	if (tr->truncate_seq) {
		size_t s1l = strlen((char *)s1), s2l = strlen((char *)s2);
		s1[MIN(s2l, s1l)] = '\0';
	}

	c = d = -1;
	while ((cp = tr_getnext(&s1)) > 0) {
		dp = tr_getnext(&s2);
		if (((cp < 0) && (c < 0)) || ((dp < 0) && (d < 0)))
			return TR_EINVAL;
		c = cp;
		d = dp;
		tr->set_tr[c] = d;
		if (tr->squeeze_seq)
			tr->set_squ[c] = 1;
	}
	return TR_OK;
}

int tr_char(tr_state_t * tr, uint8_t c) { /*return character to emit, -1 otherwise */
	assert(tr);
	if ((c == tr->previous_char) && tr->squeeze_seq && tr->set_squ[c])
		return -1;
	tr->previous_char = c;
	if (tr->delete_seq)
		return tr->set_del[c] ? -1 : c;
	return tr->set_tr[c];
}

size_t tr_block(tr_state_t * tr, uint8_t * in, uint8_t * out, size_t len) {
	size_t i = 0, j = 0;
	for (; j < len; j++) {
		int c = 0;
		if ((c = tr_char(tr, in[j])) >= 0)
			out[i++] = c;
	}
	return i;
}

tr_state_t *tr_new(void) {
	return calloc(1, sizeof(tr_state_t));
}

void tr_delete(tr_state_t * st) {
	free(st);
}

