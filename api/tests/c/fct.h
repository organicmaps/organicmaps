/*
====================================================================
Copyright (c) 2008 Ian Blumel.  All rights reserved.

FCTX (Fast C Test) Unit Testing Framework

Copyright (c) 2008, Ian Blumel (ian.blumel@gmail.com)
All rights reserved.

This license is based on the BSD License.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

    * Neither the name of, Ian Blumel, nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
====================================================================

File: fct.h
*/

#if !defined(FCT_INCLUDED__IMB)
#define FCT_INCLUDED__IMB

/* Configuration Values. You can over-ride these values in your own
header, then include this header. For example, in your file, myfct.h,

    #define FCT_DEFAULT_LOGGER "standard"
    #include "fct.h"

then if your unit tests included, myfct.h, you would default to work
with a standard logger. */

#if !defined(FCT_DEFAULT_LOGGER)
#   define FCT_DEFAULT_LOGGER  "standard"
#endif /* !FCT_DEFAULT_LOGGER */

#define FCT_VERSION_MAJOR 1
#define FCT_VERSION_MINOR 6
#define FCT_VERSION_MICRO 1

#define _FCT_QUOTEME(x) #x
#define FCT_QUOTEME(x) _FCT_QUOTEME(x)

#define FCT_VERSION_STR (FCT_QUOTEME(FCT_VERSION_MAJOR) "."\
                         FCT_QUOTEME(FCT_VERSION_MINOR) "."\
                         FCT_QUOTEME(FCT_VERSION_MICRO))

#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <float.h>
#include <math.h>
#include <ctype.h>

#define FCT_MAX_NAME           256
#define FCT_MAX_LOG_LINE       256

#define nbool_t int
#define FCT_TRUE   1
#define FCT_FALSE  0

#define FCTMIN(x, y) ( x < y) ? (x) : (y)

#ifndef __INTEL_COMPILER
/* Use regular assertions for non-Intel compilers */
#define FCT_ASSERT(expr) assert(expr)
#else
/* Silence Intel warnings on assert(expr && "str") or assert("str") */
#define FCT_ASSERT(expr) do {             \
    _Pragma("warning(push,disable:279)"); \
    assert(expr);                         \
    _Pragma("warning(pop)");              \
    } while (0)
#endif

#if defined(__cplusplus)
#define FCT_EXTERN_C extern "C"
#else
#define FCT_EXTERN_C
#endif

/* Forward declarations. The following forward declarations are required
because there is a inter-relationship between certain objects that
just can not be untwined. */
typedef struct _fct_logger_evt_t fct_logger_evt_t;
typedef struct _fct_logger_i fct_logger_i;
typedef struct _fct_logger_types_t fct_logger_types_t;
typedef struct _fct_standard_logger_t fct_standard_logger_t;
typedef struct _fct_junit_logger_t fct_junit_logger_t;
typedef struct _fct_minimal_logger_t fct_minimal_logger_t;
typedef struct _fctchk_t fctchk_t;
typedef struct _fct_test_t fct_test_t;
typedef struct _fct_ts_t fct_ts_t;
typedef struct _fctkern_t fctkern_t;

/* Forward declare some functions used throughout. */
static fct_logger_i*
fct_standard_logger_new(void);

static fct_logger_i*
fct_minimal_logger_new(void);

static fct_junit_logger_t *
fct_junit_logger_new(void);

static void
fct_logger__del(fct_logger_i *logger);

static void
fct_logger__on_chk(fct_logger_i *self, fctchk_t const *chk);

static void
fct_logger__on_test_start(fct_logger_i *logger, fct_test_t const *test);

static void
fct_logger__on_test_end(fct_logger_i *logger, fct_test_t *test);

static void
fct_logger__on_test_suite_start(fct_logger_i *logger, fct_ts_t const *ts);

static void
fct_logger__on_test_suite_end(fct_logger_i *logger, fct_ts_t const *ts);

static void
fct_logger__on_test_suite_skip(
    fct_logger_i *logger,
    char const *condition,
    char const *name
);

static void
fct_logger__on_test_skip(
    fct_logger_i *logger,
    char const *condition,
    char const *name
);


static void
fct_logger__on_warn(fct_logger_i *logger, char const *warn);



/* Explicitly indicate a no-op */
#define fct_pass()

#define fct_unused(x)  (void)(x)

/* This is just a little trick to let me put comments inside of macros. I
really only want to bother with this when we are "unwinding" the macros
for debugging purposes. */
#if defined(FCT_CONF_UNWIND)
#	define _fct_cmt(string)		{char*_=string;}
#else
#	define _fct_cmt(string)
#endif

/*
--------------------------------------------------------
UTILITIES
--------------------------------------------------------
*/


/* STDIO and STDERR redirect support */
#define FCT_PIPE_RESERVE_BYTES_DEFAULT  512
static int fct_stdout_pipe[2];
static int fct_stderr_pipe[2];
static int fct_saved_stdout;
static int fct_saved_stderr;

/* Platform indepedent pipe functions. TODO: Look to figure this out in a way
that follows the ISO C++ conformant naming convention. */
#if defined(WIN32)
#    include <io.h>
#    include <fcntl.h>
#    define _fct_pipe(_PFDS_) \
        _pipe((_PFDS_), FCT_PIPE_RESERVE_BYTES_DEFAULT, _O_TEXT)
#    define _fct_dup  _dup
#    define _fct_dup2 _dup2
#    define _fct_close _close
#    define _fct_read  _read
/* Until I can figure a better way to do this, rely on magic numbers. */
#    define STDOUT_FILENO 1
#    define STDERR_FILENO 2
#else
#    include <unistd.h>
#    define _fct_pipe  pipe
#    define _fct_dup   dup
#    define _fct_dup2  dup2
#    define _fct_close close
#    define _fct_read  read
#endif /* WIN32 */




static void
fct_switch_std_to_buffer(int std_pipe[2], FILE *out, int fileno_, int *save_handle)
{
    fflush(out);
    *save_handle = _fct_dup(fileno_);
    if ( _fct_pipe(std_pipe) != 0 )
    {
        exit(1);
    }
    _fct_dup2(std_pipe[1], fileno_);
    _fct_close(std_pipe[1]);
}


static void
fct_switch_std_to_std(FILE *out, int fileno_, int save_handle)
{
    fflush(out);
    _fct_dup2(save_handle, fileno_);
}


#define FCT_SWITCH_STDOUT_TO_BUFFER() \
    fct_switch_std_to_buffer(fct_stdout_pipe, stdout, STDOUT_FILENO, &fct_saved_stdout)
#define FCT_SWITCH_STDOUT_TO_STDOUT() \
    fct_switch_std_to_std(stdout, STDOUT_FILENO, fct_saved_stdout)
#define FCT_SWITCH_STDERR_TO_BUFFER() \
    fct_switch_std_to_buffer(fct_stderr_pipe, stderr, STDERR_FILENO, &fct_saved_stderr)
#define FCT_SWITCH_STDERR_TO_STDERR() \
    fct_switch_std_to_std(stderr, STDERR_FILENO, fct_saved_stderr)


/* Utility for truncated, safe string copies. The NUM
should be the length of DST plus the null-termintor. */
static void
fctstr_safe_cpy(char *dst, char const *src, size_t num)
{
    FCT_ASSERT( dst != NULL );
    FCT_ASSERT( src != NULL );
    FCT_ASSERT( num > 0 );
#if defined(WIN32) && _MSC_VER >= 1400
    strncpy_s(dst, num, src, _TRUNCATE);
#else
    strncpy(dst, src, num);
#endif
    dst[num-1] = '\0';
}

/* Isolate the vsnprintf implementation */
static int
fct_vsnprintf(char *buffer,
              size_t buffer_len,
              char const *format,
              va_list args)
{
    int count =0;
    /* Older microsoft compilers where not ANSI compliant with this
    function and you had to use _vsnprintf. I will assume that newer
    Microsoft Compilers start implementing vsnprintf. */
#if defined(_MSC_VER) && (_MSC_VER < 1400)
    count = _vsnprintf(buffer, buffer_len, format, args);
#elif defined(_MSC_VER) && (_MSC_VER >= 1400)
    count = vsnprintf_s(buffer, buffer_len, _TRUNCATE, format, args);
#else
    count = vsnprintf(buffer, buffer_len, format, args);
#endif
    return count;
}


/* Isolate the snprintf implemenation. */
static int
fct_snprintf(char *buffer, size_t buffer_len, char const *format, ...)
{
    int count =0;
    va_list args;
    va_start(args, format);
    count =fct_vsnprintf(buffer, buffer_len, format, args);
    va_end(args);
    return count;
}


/* Helper to for cloning strings on the heap. Returns NULL for
an out of memory condition. */
static char*
fctstr_clone(char const *s)
{
    char *k =NULL;
    size_t klen =0;
    FCT_ASSERT( s != NULL && "invalid arg");
    klen = strlen(s)+1;
    k = (char*)malloc(sizeof(char)*klen+1);
    fctstr_safe_cpy(k, s, klen);
    return k;
}


/* Clones and returns a lower case version of the original string. */
static char*
fctstr_clone_lower(char const *s)
{
    char *k =NULL;
    size_t klen =0;
    size_t i;
    if ( s == NULL )
    {
        return NULL;
    }
    klen = strlen(s)+1;
    k = (char*)malloc(sizeof(char)*klen+1);
    for ( i=0; i != klen; ++i )
    {
        k[i] = (char)tolower(s[i]);
    }
    return k;
}


/* A very, very simple "filter". This just compares the supplied prefix
against the test_str, to see if they both have the same starting
characters. If they do we return true, otherwise we return false. If the
prefix is a blank string or NULL, then it will return FCT_TRUE.*/
static nbool_t
fct_filter_pass(char const *prefix, char const *test_str)
{
    nbool_t is_match = FCT_FALSE;
    char const *prefix_p;
    char const *test_str_p;

    /* If you got nothing to test against, why test? */
    FCT_ASSERT( test_str != NULL );

    /* When the prefix is NULL or blank, we always return FCT_TRUE. */
    if ( prefix == NULL  || prefix[0] == '\0' )
    {
        return FCT_TRUE;
    }

    /* Iterate through both character arrays at the same time. We are
    going to play a game and see if we can beat the house. */
    for ( prefix_p = prefix, test_str_p = test_str;
            *prefix_p != '\0' && *test_str_p != '\0';
            ++prefix_p, ++test_str_p )
    {
        is_match = *prefix_p == *test_str_p;
        if ( !is_match )
        {
            break;   /* Quit the first time we don't match. */
        }
    }

    /* If the iterator for the test_str is pointing at the null char, and
    the iterator for the prefix string is not, then the prefix string is
    larger than the actual test string, and therefore we failed to pass the
    filter. */
    if ( *test_str_p == '\0' && *prefix_p != '\0' )
    {
        return FCT_FALSE;
    }

    /* is_match will be set to the either FCT_TRUE if we kicked of the loop
    early because our filter ran out of characters or FCT_FALSE if we
    encountered a mismatch before our filter ran out of characters. */
    return is_match;
}


/* Routine checks if two strings are equal. Taken from
http://publications.gbdirect.co.uk/c_book/chapter5/character_handling.html
*/
static int
fctstr_eq(char const *s1, char const *s2)
{
    if ( s1 == s2 )
    {
        return 1;
    }
    if ( (s1 == NULL && s2 != NULL)
            || (s1 != NULL && s2 == NULL) )
    {
        return 0;
    }
    while (*s1 == *s2)
    {
        if (*s1 == '\0')
            return 1;
        s1++;
        s2++;
    }
    /* Difference detected! */
    return 0;
}


static int
fctstr_ieq(char const *s1, char const *s2)
{
    if ( s1 == s2 )
    {
        return 1;
    }
    if ( (s1 == NULL && s2 != NULL)
            || (s1 != NULL && s2 == NULL) )
    {
        return 0;
    }
    while (tolower(*s1) == tolower(*s2))
    {
        if (*s1 == '\0')
            return 1;
        s1++;
        s2++;
    }
    /* Difference detected! */
    return 0;
}


/* Returns 1 if the STR contains the CHECK_INCL substring. NULL's
are handled, and NULL always INCLUDES NULL. This check is case
sensitive. If two strings point to the same place they are
included. */
static int
fctstr_incl(char const *str, char const *check_incl)
{
    static char const *blank_s = "";
    char const *found = NULL;
    if ( str == NULL )
    {
        str = blank_s;
    }
    if ( check_incl == NULL )
    {
        check_incl = blank_s;
    }
    if ( str == check_incl )
    {
        return 1;
    }
    found = strstr(str, check_incl);
    return found != NULL;
}


/* Does a case insensitive include check. */
static int
fctstr_iincl(char const *str, char const *check_incl)
{
    /* Going to do this with a memory allocation to save coding
    time. In the future this can be rewritten. Both clone_lower
    and _incl are NULL tolerant. */
    char *lstr = fctstr_clone_lower(str);
    char *lcheck_incl = fctstr_clone_lower(check_incl);
    int found = fctstr_incl(lstr, lcheck_incl);
    free(lstr);
    free(lcheck_incl);
    return found;
}


/* Returns true if STR starts with CHECK. NULL and NULL is consider
true. */
static int
fctstr_startswith(char const *str, char const *check)
{
    char const *sp;
    if ( str == NULL && check == NULL )
    {
        return 1;
    }
    else if ( ((str == NULL) && (check != NULL))
              || ((str != NULL) && (check == NULL)) )
    {
        return 0;
    }
    sp = strstr(str, check);
    return sp == str;
}


/* Case insenstive variant of fctstr_startswith. */
static int
fctstr_istartswith(char const *str, char const *check)
{
    /* Taking the lazy approach for now. */
    char *istr = fctstr_clone_lower(str);
    char *icheck = fctstr_clone_lower(check);
    /* TODO: check for memory. */
    int startswith = fctstr_startswith(istr, icheck);
    free(istr);
    free(icheck);
    return startswith;
}


/* Returns true if the given string ends with the given
check. Treats NULL as a blank string, and as such, will
pass the ends with (a blank string endswith a blank string). */
static int
fctstr_endswith(char const *str, char const *check)
{
    size_t check_i;
    size_t str_i;
    if ( str == NULL && check == NULL )
    {
        return 1;
    }
    else if ( ((str == NULL) && (check != NULL))
              || ((str != NULL) && (check == NULL)) )
    {
        return 0;
    }
    check_i = strlen(check);
    str_i = strlen(str);
    if ( str_i < check_i )
    {
        return 0;   /* Can't do it string is too small. */
    }
    for ( ; check_i != 0; --check_i, --str_i)
    {
        if ( str[str_i] != check[check_i] )
        {
            return 0; /* Found a case where they are not equal. */
        }
    }
    /* Exahausted check against string, can only be true. */
    return 1;
}


static int
fctstr_iendswith(char const *str, char const *check)
{
    size_t check_i;
    size_t str_i;
    if ( str == NULL && check == NULL )
    {
        return 1;
    }
    else if ( ((str == NULL) && (check != NULL))
              || ((str != NULL) && (check == NULL)) )
    {
        return 0;
    }
    check_i = strlen(check);
    str_i = strlen(str);
    if ( str_i < check_i )
    {
        return 0;   /* Can't do it string is too small. */
    }
    for ( ; check_i != 0; --check_i, --str_i)
    {
        if ( tolower(str[str_i]) != tolower(check[check_i]) )
        {
            return 0; /* Found a case where they are not equal. */
        }
    }
    /* Exahausted check against string, can only be true. */
    return 1;
}


/* Use this with the _end variant to get the

STARTSWITH ........................................ END

effect. Assumes that the line will be maxwidth in characters. The
maxwidth can't be greater than FCT_DOTTED_MAX_LEN. */
#define FCT_DOTTED_MAX_LEN  256
static void
fct_dotted_line_start(size_t maxwidth, char const *startwith)
{
    char line[FCT_DOTTED_MAX_LEN];
    size_t len =0;
    size_t line_len =0;

    memset(line, '.', sizeof(char)*maxwidth);
    len = strlen(startwith);
    line_len = FCTMIN(maxwidth-1, len);
    memcpy(line, startwith, sizeof(char)*line_len);
    if ( len < maxwidth-1)
    {
        line[len] = ' ';
    }
    line[maxwidth-1] = '\0';
    fputs(line, stdout);
}


static void
fct_dotted_line_end(char const *endswith)
{
    printf(" %s\n", endswith);
}


/*
--------------------------------------------------------
TIMER
--------------------------------------------------------
This is a low-res implementation at the moment.

We will improve this in the future, and isolate the
implementation from the rest of the code.
*/

typedef struct _fct_timer_t fct_timer_t;
struct _fct_timer_t
{
    clock_t start;
    clock_t stop;
    double duration;
};


static void
fct_timer__init(fct_timer_t *timer)
{
    FCT_ASSERT(timer != NULL);
    memset(timer, 0, sizeof(fct_timer_t));
}


static void
fct_timer__start(fct_timer_t *timer)
{
    FCT_ASSERT(timer != NULL);
    timer->start = clock();
}


static void
fct_timer__stop(fct_timer_t *timer)
{
    FCT_ASSERT(timer != NULL);
    timer->stop = clock();
    timer->duration = (double) (timer->stop - timer->start) / CLOCKS_PER_SEC;
}


/* Returns the time in seconds. */
static double
fct_timer__duration(fct_timer_t const *timer)
{
    FCT_ASSERT( timer != NULL );
    return timer->duration;
}


/*
--------------------------------------------------------
GENERIC LIST
--------------------------------------------------------
*/

/* For now we will just keep it at a linear growth rate. */
#define FCT_LIST_GROWTH_FACTOR   2

/* Starting size for the list, to keep it simple we will start
at a reasonable size. */
#define FCT_LIST_DEFAULT_START_SZ      8

/* Helper macros for quickly iterating through a list. You should be able
to do something like,

  FCT_NLIST_FOREACH_BGN(fct_logger_i*, logger, my_list)
  {
     fct_logger__on_blah(logger);
  }
  FCT_NLIST_FOREACH_END();

*/
#define FCT_NLIST_FOREACH_BGN(Type, Var, List)\
{\
   if ( List != NULL ) {\
      size_t item_i##Var;\
      size_t num_items##Var = fct_nlist__size(List);\
      for( item_i##Var =0; item_i##Var != num_items##Var; ++item_i##Var )\
      {\
         Type Var = (Type) fct_nlist__at((List), item_i##Var);

#define FCT_NLIST_FOREACH_END() }}}

/* Used to manage a list of loggers. This works mostly like
the STL vector, where the array grows as more items are
appended. */
typedef struct _fct_nlist_t fct_nlist_t;
struct _fct_nlist_t
{
    /* Item's are stored as pointers to void. */
    void **itm_list;

    /* Indicates the number of element's in the array. */
    size_t avail_itm_num;

    /* Indicates the number of actually elements in the array. */
    size_t used_itm_num;
};
typedef void (*fct_nlist_on_del_t)(void*);


/* Clears the contents of the list, and sets the list count to 0. The
actual count remains unchanged. If on_del is supplied it is executed
against each list element. */
static void
fct_nlist__clear(fct_nlist_t *list, fct_nlist_on_del_t on_del)
{
    size_t itm_i__ =0;
    FCT_ASSERT( list != NULL );
    if ( on_del != NULL )
    {
        for ( itm_i__=0; itm_i__ != list->used_itm_num; ++itm_i__ )
        {
            on_del(list->itm_list[itm_i__]);
        }
    }
    list->used_itm_num =0;
}


/* If you used init, then close with final. This is useful for
working with structures that live on the stack. */
static void
fct_nlist__final(fct_nlist_t *list, fct_nlist_on_del_t on_del)
{
    FCT_ASSERT( list != NULL );
    fct_nlist__clear(list, on_del);
    free(list->itm_list);
}


static int
fct_nlist__init2(fct_nlist_t *list, size_t start_sz)
{
    FCT_ASSERT( list != NULL );
    if ( start_sz == 0 )
    {
        list->itm_list = NULL;
    }
    else
    {
        list->itm_list = (void**)malloc(sizeof(void*)*start_sz);
        if ( list->itm_list == NULL )
        {
            return 0;
        }
    }
    /* If these are both 0, then they are equal and that means
    that the first append operation will allocate memory. The beauty
    here is that if the list remains empty, then we save a malloc.
    Empty lists are relatively common in FCT (consider an error list). */
    list->avail_itm_num = start_sz;
    list->used_itm_num =0;
    return 1;
}


/* Initializes a list. Useful for populating existing structures.
Returns 0 if there was an error allocating memory. Returns 1 otherwise. */
#define fct_nlist__init(_LIST_PTR_) \
   (fct_nlist__init2((_LIST_PTR_), FCT_LIST_DEFAULT_START_SZ))


/* Returns the number of elements within the list. */
static size_t
fct_nlist__size(fct_nlist_t const *list)
{
    FCT_ASSERT( list != NULL );
    return list->used_itm_num;
}


/* Returns the item at idx, asserts otherwise. */
static void*
fct_nlist__at(fct_nlist_t const *list, size_t idx)
{
    FCT_ASSERT( list != NULL );
    FCT_ASSERT( idx < list->used_itm_num );
    return list->itm_list[idx];
}


static void
fct_nlist__append(fct_nlist_t *list, void *itm)
{
    FCT_ASSERT( list != NULL );
    /* If we ran out of room, then the last increment should be equal to the
    available space, in this case we need to grow a little more. If this
    list started as size 0, then we should encounter the same effect as
    "running out of room." */
    if ( list->used_itm_num == list->avail_itm_num )
    {
        /* Use multiple and add, since the avail_itm_num could be 0. */
        list->avail_itm_num = list->avail_itm_num*FCT_LIST_GROWTH_FACTOR+\
                              FCT_LIST_GROWTH_FACTOR;
        list->itm_list = (void**)realloc(
                             list->itm_list, sizeof(void*)*list->avail_itm_num
                         );
        FCT_ASSERT( list->itm_list != NULL && "memory check");
    }

    list->itm_list[list->used_itm_num] = itm;
    ++(list->used_itm_num);
}



/*
-----------------------------------------------------------
A SINGLE CHECK
-----------------------------------------------------------
This defines a single check. It indicates what the check was,
and where it occurred. A "Test" object will have-a bunch
of "checks".
*/

struct _fctchk_t
{
    /* This string that represents the condition. */
    char cndtn[FCT_MAX_LOG_LINE];

    /* These indicate where the condition occurred. */
    char file[FCT_MAX_LOG_LINE];

    int lineno;

    nbool_t is_pass;

    /* This is a message that we can "format into", if
    no format string is specified this should be
    equivalent to the cntdn. */
    char msg[FCT_MAX_LOG_LINE];
};

#define fctchk__is_pass(_CHK_) ((_CHK_)->is_pass)
#define fctchk__file(_CHK_)    ((_CHK_)->file)
#define fctchk__lineno(_CHK_)  ((_CHK_)->lineno)
#define fctchk__cndtn(_CHK_)   ((_CHK_)->cndtn)
#define fctchk__msg(_CHK_)     ((_CHK_)->msg)

static fctchk_t*
fctchk_new(int is_pass,
           char const *cndtn,
           char const *file,
           int lineno,
           char const *format,
           va_list args)
{
    fctchk_t *chk = NULL;

    FCT_ASSERT( cndtn != NULL );
    FCT_ASSERT( file != NULL );
    FCT_ASSERT( lineno > 0 );

    chk = (fctchk_t*)calloc(1, sizeof(fctchk_t));
    if ( chk == NULL )
    {
        return NULL;
    }

    fctstr_safe_cpy(chk->cndtn, cndtn, FCT_MAX_LOG_LINE);
    fctstr_safe_cpy(chk->file, file, FCT_MAX_LOG_LINE);
    chk->lineno = lineno;

    chk->is_pass =is_pass;

    if ( format != NULL )
    {
        fct_vsnprintf(chk->msg, FCT_MAX_LOG_LINE, format, args);
    }
    else
    {
        /* Default to make the condition be the message, if there was no format
        specified. */
        fctstr_safe_cpy(chk->msg, cndtn, FCT_MAX_LOG_LINE);
    }

    return chk;
}


/* Cleans up a "check" object. If the `chk` is NULL, this function does
nothing. */
static void
fctchk__del(fctchk_t *chk)
{
    if ( chk == NULL )
    {
        return;
    }
    free( chk );
}


/*
-----------------------------------------------------------
A TEST
-----------------------------------------------------------
A suite will have-a list of tests. Where each test will have-a
list of failed and passed checks.
*/

struct _fct_test_t
{
    /* List of failed and passed "checks" (fctchk_t). Two seperate
    lists make it faster to determine how many checks passed and how
    many checks failed. */
    fct_nlist_t failed_chks;
    fct_nlist_t passed_chks;

    /* To store the test run time */
    fct_timer_t timer;

    /* The name of the test case. */
    char name[FCT_MAX_NAME];
};

#define fct_test__name(_TEST_) ((_TEST_)->name)

/* Clears the failed tests ... partly for internal testing. */
#define fct_test__clear_failed(test) \
    fct_nlist__clear(test->failed_chks, (fct_nlist_on_del_t)fctchk__del);\


static void
fct_test__del(fct_test_t *test)
{
    if (test == NULL )
    {
        return;
    }
    fct_nlist__final(&(test->passed_chks), (fct_nlist_on_del_t)fctchk__del);
    fct_nlist__final(&(test->failed_chks), (fct_nlist_on_del_t)fctchk__del);
    free(test);
}


static fct_test_t*
fct_test_new(char const *name)
{
    nbool_t ok =FCT_FALSE;
    fct_test_t *test =NULL;

    test = (fct_test_t*)malloc(sizeof(fct_test_t));
    if ( test == NULL )
    {
        return NULL;
    }

    fctstr_safe_cpy(test->name, name, FCT_MAX_NAME);

    /* Failures are an exception, so lets not allocate up
    the list until we need to. */
    fct_nlist__init2(&(test->failed_chks), 0);
    if (!fct_nlist__init(&(test->passed_chks)))
    {
        ok =FCT_FALSE;
        goto finally;
    }

    fct_timer__init(&(test->timer));

    ok =FCT_TRUE;
finally:
    if ( !ok )
    {
        fct_test__del(test);
        test =NULL;
    }
    return test;
}


static void
fct_test__start_timer(fct_test_t *test)
{
    FCT_ASSERT( test != NULL );
    fct_timer__start(&(test->timer));
}


static void
fct_test__stop_timer(fct_test_t *test)
{
    FCT_ASSERT( test != NULL );
    fct_timer__stop(&(test->timer));
}


static double
fct_test__duration(fct_test_t const *test)
{
    FCT_ASSERT( test != NULL );
    return fct_timer__duration(&(test->timer));
}


static nbool_t
fct_test__is_pass(fct_test_t const *test)
{
    FCT_ASSERT( test != NULL );
    return fct_nlist__size(&(test->failed_chks)) == 0;
}


static void
fct_test__add(fct_test_t *test, fctchk_t *chk)
{

    FCT_ASSERT( test != NULL );
    FCT_ASSERT( chk != NULL );

    if ( fctchk__is_pass(chk) )
    {
        fct_nlist__append(&(test->passed_chks), (void*)chk);
    }
    else
    {
        fct_nlist__append(&(test->failed_chks), (void*)chk);
    }
}

/* Returns the number of checks made throughout the test. */
static size_t
fct_test__chk_cnt(fct_test_t const *test)
{
    FCT_ASSERT( test != NULL );
    return fct_nlist__size(&(test->failed_chks)) \
           + fct_nlist__size(&(test->passed_chks));
}


/*
-----------------------------------------------------------
TEST SUITE (TS)
-----------------------------------------------------------
*/


/* The different types of 'modes' that a test suite can be in.

While the test suite is iterating through all the tests, its "State"
can change from "setup mode", to "test mode" to "tear down" mode.
These help to indicate what mode are currently in. Think of it as a
basic FSM.

            if the count was 0                                 end
           +--------->---------------------> ending_mode-----+-+
           |                                       ^         |
           ^                                       |         ^
start      |                              [if no more tests] |
  |        |                                       |         |
  +-count_mode -> setup_mode -> test_mode -> teardown_mode->-+
                   |  ^                           |          |
                   |  +-----------<---------------+          |
                   +----------->---[if fct_req fails]--------+

*/
enum ts_mode
{
    ts_mode_cnt,         /* To setup when done counting. */
    ts_mode_setup,       /* To test when done setup. */
    ts_mode_teardown,    /* To ending mode, when no more tests. */
    ts_mode_test,        /* To tear down mode. */
    ts_mode_ending,      /* To ... */
    ts_mode_end,         /* .. The End. */
    ts_mode_abort        /* Abort */
};

/* Types of states the test could be in. */
typedef enum
{
    fct_test_status_SUCCESS,
    fct_test_status_FAILURE
} fct_test_status;


struct _fct_ts_t
{
    /* For counting our 'current' test number, and the total number of
    tests. */
    int  curr_test_num;
    int  total_test_num;

    /* Keeps track of the current state of the object while it is walking
    through its "FSM" */
    enum ts_mode mode;

    /* The name of the test suite. */
    char name[FCT_MAX_NAME];

    /* List of tests that where executed within the test suite. */
    fct_nlist_t test_list;
};


#define fct_ts__is_setup_mode(ts)     ((ts)->mode == ts_mode_setup)
#define fct_ts__is_teardown_mode(ts)  ((ts)->mode == ts_mode_teardown)
#define fct_ts__is_test_mode(ts)      ((ts)->mode == ts_mode_test)
#define fct_ts__is_ending_mode(ts)    ((ts)->mode == ts_mode_ending)
#define fct_ts__is_end(ts)            ((ts)->mode == ts_mode_end)
#define fct_ts__is_cnt_mode(ts)       ((ts)->mode == ts_mode_cnt)
#define fct_ts__is_abort_mode(ts)     ((ts)->mode == ts_mode_abort)

/* This cndtn is set when we have iterated through all the tests, and
there was nothing more to do. */
#define fct_ts__ending(ts)          ((ts)->mode = ts_mode_ending)

/* Flag a test suite as complete. It will no longer accept any more tests. */
#define fct_ts__end(ts)  ((ts)->mode = ts_mode_end)

#define fct_ts__name(ts)              ((ts)->name)


static void
fct_ts__del(fct_ts_t *ts)
{
    if ( ts == NULL )
    {
        return;
    }
    fct_nlist__final(&(ts->test_list), (fct_nlist_on_del_t)fct_test__del);
    free(ts);
}

static fct_ts_t *
fct_ts_new(char const *name)
{
    fct_ts_t *ts =NULL;
    ts = (fct_ts_t*)calloc(1, sizeof(fct_ts_t));
    FCT_ASSERT( ts != NULL );

    fctstr_safe_cpy(ts->name, name, FCT_MAX_NAME);
    ts->mode = ts_mode_cnt;
    fct_nlist__init(&(ts->test_list));
    return ts;
}



static nbool_t
fct_ts__is_more_tests(fct_ts_t const *ts)
{
    FCT_ASSERT( ts != NULL );
    FCT_ASSERT( !fct_ts__is_end(ts) );
    return ts->curr_test_num < ts->total_test_num;
}


/* Indicates that we have started a test case. */
static void
fct_ts__test_begin(fct_ts_t *ts)
{
    FCT_ASSERT( !fct_ts__is_end(ts) );
    ++(ts->curr_test_num);
}


/* Takes OWNERSHIP of a test object, and warehouses it for later stat
generation. */
static void
fct_ts__add_test(fct_ts_t *ts, fct_test_t *test)
{
    FCT_ASSERT( ts != NULL && "invalid arg");
    FCT_ASSERT( test != NULL && "invalid arg");
    FCT_ASSERT( !fct_ts__is_end(ts) );
    fct_nlist__append(&(ts->test_list), test);
}


static void
fct_ts__test_end(fct_ts_t *ts)
{
    FCT_ASSERT( ts != NULL );
    /* After a test has completed, move to teardown mode. */
    ts->mode = ts_mode_teardown;
}


/* Increments the internal count by 1. */
static void
fct_ts__inc_total_test_num(fct_ts_t *ts)
{
    FCT_ASSERT( ts != NULL );
    FCT_ASSERT( fct_ts__is_cnt_mode(ts) );
    FCT_ASSERT( !fct_ts__is_end(ts) );
    ++(ts->total_test_num);
}


/* Flags the end of the setup, which implies we are going to move into
setup mode. You must be already in setup mode for this to work! */
static void
fct_ts__setup_end(fct_ts_t *ts)
{
    if ( ts->mode != ts_mode_abort )
    {
        ts->mode = ts_mode_test;
    }
}


static fct_test_t *
fct_ts__make_abort_test(fct_ts_t *ts)
{
    char setup_testname[FCT_MAX_LOG_LINE+1] = {'\0'};
    char const *suitename = fct_ts__name(ts);
    fct_snprintf(setup_testname, FCT_MAX_LOG_LINE, "setup_%s", suitename);
    return fct_test_new(setup_testname);
}

/* Flags a pre-mature abort of a setup (like a failed fct_req). */
static void
fct_ts__setup_abort(fct_ts_t *ts)
{
    FCT_ASSERT( ts != NULL );
    ts->mode = ts_mode_abort;
}

/* Flags the end of the teardown, which implies we are going to move
into setup mode (for the next 'iteration'). */
static void
fct_ts__teardown_end(fct_ts_t *ts)
{
    if ( ts->mode == ts_mode_abort )
    {
        return; /* Because we are aborting . */
    }
    /* We have to decide if we should keep on testing by moving into tear down
    mode or if we have reached the real end and should be moving into the
    ending mode. */
    if ( fct_ts__is_more_tests(ts) )
    {
        ts->mode = ts_mode_setup;
    }
    else
    {
        ts->mode = ts_mode_ending;
    }
}


/* Flags the end of the counting, and proceeding to the first setup.
Consider the special case when a test suite has NO tests in it, in
that case we will have a current count that is zero, in which case
we can skip right to 'ending'. */
static void
fct_ts__cnt_end(fct_ts_t *ts)
{
    FCT_ASSERT( ts != NULL );
    FCT_ASSERT( fct_ts__is_cnt_mode(ts) );
    FCT_ASSERT( !fct_ts__is_end(ts) );
    if (ts->total_test_num == 0  )
    {
        ts->mode = ts_mode_ending;
    }
    else
    {
        ts->mode = ts_mode_setup;
    }
}


static nbool_t
fct_ts__is_test_cnt(fct_ts_t const *ts, int test_num)
{
    FCT_ASSERT( ts != NULL );
    FCT_ASSERT( 0 <= test_num );
    FCT_ASSERT( test_num < ts->total_test_num );
    FCT_ASSERT( !fct_ts__is_end(ts) );

    /* As we roll through the tests we increment the count. With this
    count we can decide if we need to execute a test or not. */
    return test_num == ts->curr_test_num;
}


/* Returns the # of tests on the FCT TS object. This is the actual
# of tests executed. */
static size_t
fct_ts__tst_cnt(fct_ts_t const *ts)
{
    FCT_ASSERT( ts != NULL );
    FCT_ASSERT(
        fct_ts__is_end(ts)
        && "can't count number of tests executed until the test suite ends"
    );
    return fct_nlist__size(&(ts->test_list));
}


/* Returns the # of tests in the TS object that passed. */
static size_t
fct_ts__tst_cnt_passed(fct_ts_t const *ts)
{
    size_t tally =0;

    FCT_ASSERT( ts != NULL );
    FCT_ASSERT( fct_ts__is_end(ts) );

    FCT_NLIST_FOREACH_BGN(fct_test_t*, test, &(ts->test_list))
    {
        if ( fct_test__is_pass(test) )
        {
            tally += 1;
        }
    }
    FCT_NLIST_FOREACH_END();
    return tally;
}


/* Returns the # of checks made throughout a test suite. */
static size_t
fct_ts__chk_cnt(fct_ts_t const *ts)
{
    size_t tally =0;

    FCT_ASSERT( ts != NULL );

    FCT_NLIST_FOREACH_BGN(fct_test_t *, test, &(ts->test_list))
    {
        tally += fct_test__chk_cnt(test);
    }
    FCT_NLIST_FOREACH_END();
    return tally;
}

/* Currently the duration is simply a sum of all the tests. */
static double
fct_ts__duration(fct_ts_t const *ts)
{
    double tally =0.0;
    FCT_ASSERT( ts != NULL );
    FCT_NLIST_FOREACH_BGN(fct_test_t *, test, &(ts->test_list))
    {
        tally += fct_test__duration(test);
    }
    FCT_NLIST_FOREACH_END();
    return tally;
}


/*
--------------------------------------------------------
FCT COMMAND LINE OPTION INITIALIZATION (fctcl_init)
--------------------------------------------------------

Structure used for command line initialization. To keep it clear that we do
not delete the char*'s present on this structure.
*/


typedef enum
{
    FCTCL_STORE_UNDEFINED,
    FCTCL_STORE_TRUE,
    FCTCL_STORE_VALUE
} fctcl_store_t;


typedef struct _fctcl_init_t
{
    /* What to parse for this option. --long versus -s. */
    char const *long_opt;     /* i.e. --help */
    char const *short_opt;    /* i.e. -h */

    /* What action to take when the option is activated. */
    fctcl_store_t action;

    /* The help string for the action. */
    char const *help;
} fctcl_init_t;


/* Use when defining the option list. */
#define FCTCL_INIT_NULL  \
    {NULL, NULL, FCTCL_STORE_UNDEFINED, NULL}


/*
--------------------------------------------------------
FCT COMMAND LINE OPTION (fctcl)
--------------------------------------------------------

Specifies the command line configuration options. Use this
to help initialize the fct_clp (command line parser).
*/


/* Handy strings for storing "true" and "false". We can reference
these strings throughout the parse operation and not have to
worry about dealing with memory. */
#define FCTCL_TRUE_STR "1"


typedef struct _fctcl_t
{
    /* What to parse for this option. --long versus -s. */
    char *long_opt;     /* i.e. --help */
    char *short_opt;    /* i.e. -h */

    /* What action to take when the option is activated. */
    fctcl_store_t action;

    /* The help string for the action. */
    char *help;

    /* The result. */
    char *value;
} fctcl_t;


#define fctcl_new()  ((fctcl_t*)calloc(1, sizeof(fctcl_t)))


static void
fctcl__del(fctcl_t *clo)
{
    if ( clo == NULL )
    {
        return;
    }
    if ( clo->long_opt )
    {
        free(clo->long_opt);
    }
    if ( clo->short_opt)
    {
        free(clo->short_opt);
    }
    if ( clo->value )
    {
        free(clo->value);
    }
    if ( clo->help )
    {
        free(clo->help);
    }
    free(clo);
}


static fctcl_t*
fctcl_new2(fctcl_init_t const *clo_init)
{
    fctcl_t *clone = NULL;
    int ok =0;
    clone = fctcl_new();
    if ( clone == NULL )
    {
        return NULL;
    }
    clone->action = clo_init->action;
    if ( clo_init->help == NULL )
    {
        clone->help = NULL;
    }
    else
    {
        clone->help = fctstr_clone(clo_init->help);
        if ( clone->help == NULL )
        {
            ok =0;
            goto finally;
        }
    }
    if ( clo_init->long_opt == NULL )
    {
        clone->long_opt = NULL;
    }
    else
    {
        clone->long_opt = fctstr_clone(clo_init->long_opt);
        if ( clone->long_opt == NULL )
        {
            ok = 0;
            goto finally;
        }
    }
    if ( clo_init->short_opt == NULL )
    {
        clone->short_opt = NULL;
    }
    else
    {
        clone->short_opt = fctstr_clone(clo_init->short_opt);
        if ( clone->short_opt == NULL )
        {
            ok =0;
            goto finally;
        }
    }
    ok = 1;
finally:
    if ( !ok )
    {
        fctcl__del(clone);
        clone = NULL;
    }
    return clone;
}


static int
fctcl__is_option(fctcl_t const *clo, char const *option)
{
    FCT_ASSERT( clo != NULL );
    if ( option == NULL )
    {
        return 0;
    }
    return ((clo->long_opt != NULL
             && fctstr_eq(clo->long_opt, option))
            ||
            (clo->short_opt != NULL
             && fctstr_eq(clo->short_opt, option))
           );
}


#define fctcl__set_value(_CLO_, _VAL_) \
    (_CLO_)->value = fctstr_clone((_VAL_));

/*
--------------------------------------------------------
FCT COMMAND PARSER (fct_clp)
--------------------------------------------------------
*/

#define FCT_CLP_MAX_ERR_MSG_LEN     256

typedef struct _fct_clp_t
{
    /* List of command line options. */
    fct_nlist_t clo_list;

    /* List of parameters (not options). */
    fct_nlist_t param_list;

    char error_msg[FCT_CLP_MAX_ERR_MSG_LEN];
    int is_error;
} fct_clp_t;


static void
fct_clp__final(fct_clp_t *clp)
{
    fct_nlist__final(&(clp->clo_list), (fct_nlist_on_del_t)fctcl__del);
    fct_nlist__final(&(clp->param_list), (fct_nlist_on_del_t)free);
}


/* Add an configuration options. */
static int
fct_clp__add_options(fct_clp_t *clp, fctcl_init_t const *options)
{
    fctcl_init_t const *pclo =NULL;
    int ok;
    for ( pclo = options; pclo->action != FCTCL_STORE_UNDEFINED; ++pclo )
    {
        fctcl_t *cpy = fctcl_new2(pclo);
        if ( cpy == NULL )
        {
            ok = 0;
            goto finally;
        }
        fct_nlist__append(&(clp->clo_list), (void*)cpy);
    }
    ok =1;
finally:
    return ok;
}

/* Returns false if we ran out of memory. */
static int
fct_clp__init(fct_clp_t *clp, fctcl_init_t const *options)
{
    int ok =0;
    FCT_ASSERT( clp != NULL );
    /* It is just much saner to manage a clone of the options. Then we know
    who is in charge of the memory. */
    ok = fct_nlist__init(&(clp->clo_list));
    if ( !ok )
    {
        goto finally;
    }
    if ( options != NULL )
    {
        ok = fct_clp__add_options(clp, options);
        if ( !ok )
        {
            goto finally;
        }
    }
    ok = fct_nlist__init(&(clp->param_list));
    if ( !ok )
    {
        goto finally;
    }
    ok =1;
finally:
    if ( !ok )
    {
        fct_clp__final(clp);
    }
    return ok;
}


/* Parses the command line arguments. Use fct_clp__is_error and
fct_clp__get_error to figure out if something went awry. */
static void
fct_clp__parse(fct_clp_t *clp, int argc, char const *argv[])
{
    int argi =1;
    int is_option =0;
    char *arg =NULL;
    char *token =NULL;
    char *next_token =NULL;

    clp->error_msg[0] = '\0';
    clp->is_error =0;

    while ( argi < argc )
    {
        is_option =0;
        token =NULL;
        next_token = NULL;
        arg = fctstr_clone(argv[argi]);

#if defined(_MSC_VER) && _MSC_VER > 1300
        token = strtok_s(arg, "=", &next_token);
#else
        token = strtok(arg, "=");
        next_token = strtok(NULL, "=");
#endif

        FCT_NLIST_FOREACH_BGN(fctcl_t*, pclo, &(clp->clo_list))
        {
            /* Need to reset for each search. strtok below is destructive. */
            if ( fctcl__is_option(pclo, token) )
            {
                is_option =1;
                if ( pclo->action == FCTCL_STORE_VALUE )
                {
                    /* If this is --xxxx=value then the next strtok should succeed.
                    Otherwise, we need to chew up the next argument. */
                    if ( next_token != NULL && strlen(next_token) > 0 )
                    {
                        fctcl__set_value(pclo, next_token);
                    }
                    else
                    {
                        ++argi; /* Chew up the next value */
                        if ( argi >= argc )
                        {
                            /* error */
                            fct_snprintf(
                                clp->error_msg,
                                FCT_CLP_MAX_ERR_MSG_LEN,
                                "missing argument for %s",
                                token
                            );
                            clp->is_error =1;
                            break;
                        }
                        fctcl__set_value(pclo, argv[argi]);
                    }
                }
                else if (pclo->action == FCTCL_STORE_TRUE)
                {
                    fctcl__set_value(pclo, FCTCL_TRUE_STR);
                }
                else
                {
                    FCT_ASSERT("undefined action requested");
                }
                break;  /* No need to parse this argument further. */
            }
        }
        FCT_NLIST_FOREACH_END();
        /* If we have an error, exit. */
        if ( clp->is_error )
        {
            break;
        }
        /* If we walked through all the options, and didn't find
        anything, then we must have a parameter. Forget the fact that
        an unknown option will be treated like a parameter... */
        if ( !is_option )
        {
            fct_nlist__append(&(clp->param_list), arg);
            arg =NULL;  /* Owned by the nlist */
        }
        ++argi;
        if ( arg != NULL )
        {
            free(arg);
            arg =NULL;
        }
    }
}


static fctcl_t const*
fct_clp__get_clo(fct_clp_t const *clp, char const *option)
{
    fctcl_t const *found =NULL;

    FCT_NLIST_FOREACH_BGN(fctcl_t const*, pclo, &(clp->clo_list))
    {
        if ( fctcl__is_option(pclo, option) )
        {
            found = pclo;
            break;
        }
    }
    FCT_NLIST_FOREACH_END();
    return found;
}


#define fct_clp__optval(_CLP_, _OPTION_) \
    fct_clp__optval2((_CLP_), (_OPTION_), NULL)


/* Returns the value parsed at the command line, and equal to OPTION.
If the value wasn't parsed, the DEFAULT_VAL is returned instead. */
static char const*
fct_clp__optval2(fct_clp_t *clp, char const *option, char const *default_val)
{
    fctcl_t const *clo =NULL;
    FCT_ASSERT( clp != NULL );
    FCT_ASSERT( option != NULL );
    clo = fct_clp__get_clo(clp, option);
    if ( clo == NULL || clo->value == NULL)
    {
        return default_val;
    }
    return clo->value;
}



/* Mainly used for unit tests. */
static int
fct_clp__is_param(fct_clp_t *clp, char const *param)
{
    if ( clp == NULL || param == NULL )
    {
        return 0;
    }
    FCT_NLIST_FOREACH_BGN(char *, aparam, &(clp->param_list))
    {
        if ( fctstr_eq(aparam, param) )
        {
            return 1;
        }
    }
    FCT_NLIST_FOREACH_END();
    return 0;
}


#define fct_clp__is_error(_CLP_) ((_CLP_)->is_error)
#define fct_clp__get_error(_CLP_) ((_CLP_)->error_msg);

#define fct_clp__num_clo(_CLP_) \
    (fct_nlist__size(&((_CLP_)->clo_list)))

#define fct_clp__param_cnt(_CLP_) \
    (fct_nlist__size(&((_CLP_)->param_list)))

/* Returns a *reference* to the parameter at _IDX_. Do not modify
its contents. */
#define fct_clp__param_at(_CLP_, _IDX_) \
    ((char const*)fct_nlist__at(&((_CLP_)->param_list), (_IDX_)))


/* Returns true if the given option was on the command line.
Use either the long or short option name to check against. */
#define fct_clp__is(_CLP_, _OPTION_) \
    (fct_clp__optval((_CLP_), (_OPTION_)) != NULL)



/*
--------------------------------------------------------
FCT NAMESPACE
--------------------------------------------------------

The macros below start to pollute the watch window with
lots of "system" variables. This NAMESPACE is an
attempt to hide all the "system" variables in one place.
*/
typedef struct _fct_namespace_t
{
    /* The currently active test suite. */
    fct_ts_t *ts_curr;
    int ts_is_skip_suite;
    char const *ts_skip_cndtn;

    /* Current test name. */
    char const* curr_test_name;
    fct_test_t *curr_test;
    const char *test_skip_cndtn;
    int test_is_skip;

    /* Counts the number of tests in a test suite. */
    int test_num;

    /* Set at the end of the test suites. */
    size_t num_total_failed;
} fct_namespace_t;


static void
fct_namespace_init(fct_namespace_t *ns)
{
    FCT_ASSERT( ns != NULL && "invalid argument!");
    memset(ns, 0, sizeof(fct_namespace_t));
}


/*
--------------------------------------------------------
FCT KERNAL
--------------------------------------------------------

The "fctkern" is a singleton that is defined throughout the
system.
*/

struct _fctkern_t
{
    /* Holds variables used throughout MACRO MAGIC. In order to reduce
    the "noise" in the watch window during a debug trace. */
    fct_namespace_t ns;

    /* Command line parsing. */
    fct_clp_t cl_parser;

    /* Hold onto the command line arguments. */
    int cl_argc;
    char const **cl_argv;
    /* Track user options. */
    fctcl_init_t const *cl_user_opts;

    /* Tracks the delay parsing. */
    int cl_is_parsed;

    /* This is an list of loggers that can be used in the fct system. */
    fct_nlist_t logger_list;

    /* Array of custom types, you have built-in system ones and you
    have optionally supplied user ones.. */
    fct_logger_types_t *lt_usr;
    fct_logger_types_t *lt_sys;

    /* This is a list of prefix's that can be used to determine if a
    test is should be run or not. */
    fct_nlist_t prefix_list;

    /* This is a list of test suites that where generated throughout the
    testing process. */
    fct_nlist_t ts_list;

    /* Records what we expect to fail. */
    size_t num_expected_failures;
};


#define FCT_OPT_VERSION       "--version"
#define FCT_OPT_VERSION_SHORT "-v"
#define FCT_OPT_HELP          "--help"
#define FCT_OPT_HELP_SHORT    "-h"
#define FCT_OPT_LOGGER        "--logger"
#define FCT_OPT_LOGGER_SHORT  "-l"
static fctcl_init_t FCT_CLP_OPTIONS[] =
{
    /* Totally unsafe, since we are assuming we can clean out this data,
    what I need to do is have an "initialization" object, full of
    const objects. But for now, this should work. */
    {
        FCT_OPT_VERSION,
        FCT_OPT_VERSION_SHORT,
        FCTCL_STORE_TRUE,
        "Displays the FCTX version number and exits."
    },
    {
        FCT_OPT_HELP,
        FCT_OPT_HELP_SHORT,
        FCTCL_STORE_TRUE,
        "Shows this help."
    },
    {
        FCT_OPT_LOGGER,
        FCT_OPT_LOGGER_SHORT,
        FCTCL_STORE_VALUE,
        NULL
    },
    FCTCL_INIT_NULL /* Sentinel */
};

typedef fct_logger_i* (*fct_logger_new_fn)(void);
struct _fct_logger_types_t
{
    char const *name;
    fct_logger_new_fn logger_new_fn;
    char const *desc;
};

static fct_logger_types_t FCT_LOGGER_TYPES[] =
{
    {
        "standard",
        (fct_logger_new_fn)fct_standard_logger_new,
        "the basic fctx logger"
    },
    {
        "minimal",
        (fct_logger_new_fn)fct_minimal_logger_new,
        "the least amount of logging information."
    },
    {
        "junit",
        (fct_logger_new_fn)fct_junit_logger_new,
        "junit compatable xml"
    },
    {NULL, (fct_logger_new_fn)NULL, NULL} /* Sentinel */
};


/* Returns the number of filters defined for the fct kernal. */
#define fctkern__filter_cnt(_NK_) (fct_nlist__size(&((_NK_)->prefix_list)))


static void
fctkern__add_logger(fctkern_t *nk, fct_logger_i *logger_owns)
{
    FCT_ASSERT(nk != NULL && "invalid arg");
    FCT_ASSERT(logger_owns != NULL && "invalid arg");
    fct_nlist__append(&(nk->logger_list), logger_owns);
}


static void
fctkern__write_help(fctkern_t *nk, FILE *out)
{
    fct_clp_t *clp = &(nk->cl_parser);
    fprintf(out, "test.exe [options] prefix_filter ...\n\n");
    FCT_NLIST_FOREACH_BGN(fctcl_t*, clo, &(clp->clo_list))
    {
        if ( clo->short_opt != NULL )
        {
            fprintf(out, "%s, %s\n", clo->short_opt, clo->long_opt);
        }
        else
        {
            fprintf(out, "%s\n", clo->long_opt);
        }
        if ( !fctstr_ieq(clo->long_opt, FCT_OPT_LOGGER) )
        {
            /* For now lets not get to fancy with the text wrapping. */
            fprintf(out, "  %s\n", clo->help);
        }
        else
        {
            fct_logger_types_t *types[2];
            int type_i;
            fct_logger_types_t *itr;
            types[0] = nk->lt_sys;
            types[1] = nk->lt_usr;
            fputs("  Sets the logger. The types of loggers currently "
                  "available are,\n", out);
            for (type_i =0; type_i != 2; ++type_i )
            {
                for ( itr=types[type_i]; itr && itr->name != NULL; ++itr )
                {
                    fprintf(out, "   =%s : %s\n", itr->name, itr->desc);
                }
            }
            fprintf(out, "  default is '%s'.\n", FCT_DEFAULT_LOGGER);
        }
    }
    FCT_NLIST_FOREACH_END();
    fputs("\n", out);
}


/* Appends a prefix filter that is used to determine if a test can
be executed or not. If the test starts with the same characters as
the prefix, then it should be "runnable". The prefix filter must be
a non-NULL, non-Blank string. */
static void
fctkern__add_prefix_filter(fctkern_t *nk, char const *prefix_filter)
{
    char *filter =NULL;
    size_t filter_len =0;
    FCT_ASSERT( nk != NULL && "invalid arg" );
    FCT_ASSERT( prefix_filter != NULL && "invalid arg" );
    FCT_ASSERT( strlen(prefix_filter) > 0 && "invalid arg" );
    /* First we make a copy of the prefix, then we store it away
    in our little list. */
    filter_len = strlen(prefix_filter);
    filter = (char*)malloc(sizeof(char)*(filter_len+1));
    fctstr_safe_cpy(filter, prefix_filter, filter_len+1);
    fct_nlist__append(&(nk->prefix_list), (void*)filter);
}


/* Cleans up the contents of a fctkern. NULL does nothing. */
static void
fctkern__final(fctkern_t *nk)
{
    if ( nk == NULL )
    {
        return;
    }
    fct_clp__final(&(nk->cl_parser));
    fct_nlist__final(&(nk->logger_list), (fct_nlist_on_del_t)fct_logger__del);
    /* The prefix list is a list of malloc'd strings. */
    fct_nlist__final(&(nk->prefix_list), (fct_nlist_on_del_t)free);
    fct_nlist__final(&(nk->ts_list), (fct_nlist_on_del_t)fct_ts__del);
}


#define fctkern__cl_is_parsed(_NK_) ((_NK_)->cl_is_parsed)


static int
fctkern__cl_is(fctkern_t *nk, char const *opt_str)
{
    FCT_ASSERT( opt_str != NULL );
    return opt_str[0] != '\0'
           && fct_clp__is(&(nk->cl_parser), opt_str);
}


/* Returns the command line value given by OPT_STR. If OPT_STR was not defined
at the command line, DEF_STR is returned (you can use NULL for the DEF_STR).
The result returned should not be mofidied, and MAY even be the same pointer
to DEF_STR. */
static char const *
fctkern__cl_val2(fctkern_t *nk, char const *opt_str, char const *def_str)
{
    FCT_ASSERT( opt_str != NULL );
    if ( nk == NULL )
    {
        return NULL;
    }
    return fct_clp__optval2(&(nk->cl_parser), opt_str, def_str);
}


/* Selects a logger from the list based on the selection name.
May return NULL if the name doesn't exist in the list. */
static fct_logger_i*
fckern_sel_log(fct_logger_types_t *search, char const *sel_logger)
{
    fct_logger_types_t *iter;
    FCT_ASSERT(search != NULL);
    FCT_ASSERT(sel_logger != NULL);
    FCT_ASSERT(strlen(sel_logger) > 0);
    for ( iter = search; iter->name != NULL; ++iter)
    {
        if ( fctstr_ieq(iter->name, sel_logger) )
        {
            return iter->logger_new_fn();
        }
    }
    return NULL;
}

static int
fctkern__cl_parse_config_logger(fctkern_t *nk)
{
    fct_logger_i *logger =NULL;
    char const *sel_logger =NULL;
    char const *def_logger =FCT_DEFAULT_LOGGER;
    sel_logger = fctkern__cl_val2(nk, FCT_OPT_LOGGER, def_logger);
    FCT_ASSERT(sel_logger != NULL && "should never be NULL");
    /* First search the user selected types, then search the
    built-in types. */
    if ( nk->lt_usr != NULL )
    {
        logger = fckern_sel_log(nk->lt_usr, sel_logger);
    }
    if ( nk->lt_sys != NULL && logger == NULL )
    {
        logger = fckern_sel_log(nk->lt_sys, sel_logger);
    }
    if ( logger == NULL )
    {
        /* No logger configured, you must have supplied an invalid selection. */
        fprintf(stderr, "error: unknown logger selected - '%s'", sel_logger);
        return 0;
    }
    fctkern__add_logger(nk, logger);
    logger = NULL;  /* owned by nk. */
    return 1;
}



/* Call this if you want to (re)parse the command line options with a new
set of options. Returns -1 if you are to abort with EXIT_SUCCESS, returns
0 if you are to abort with EXIT_FAILURE and returns 1 if you are to continue. */
static int
fctkern__cl_parse(fctkern_t *nk)
{
    int status =0;
    size_t num_params =0;
    size_t param_i =0;
    if ( nk == NULL )
    {
        return 0;
    }
    if ( nk->cl_user_opts != NULL )
    {
        if ( !fct_clp__add_options(&(nk->cl_parser), nk->cl_user_opts) )
        {
            status =0;
            goto finally;
        }
    }
    /* You want to add the "house options" after the user defined ones. The
    options are stored as a list so it means that any option listed after
    the above ones won't get parsed. */
    if ( !fct_clp__add_options(&(nk->cl_parser), FCT_CLP_OPTIONS) )
    {
        status =0;
        goto finally;
    }
    fct_clp__parse(&(nk->cl_parser), nk->cl_argc, nk->cl_argv);
    if ( fct_clp__is_error(&(nk->cl_parser)) )
    {
        char *err = fct_clp__get_error(&(nk->cl_parser));
        fprintf(stderr, "error: %s", err);
        status =0;
        goto finally;
    }
    num_params = fct_clp__param_cnt(&(nk->cl_parser));
    for ( param_i =0; param_i != num_params; ++param_i )
    {
        char const *param = fct_clp__param_at(&(nk->cl_parser), param_i);
        fctkern__add_prefix_filter(nk, param);
    }
    if ( fctkern__cl_is(nk, FCT_OPT_VERSION) )
    {
        (void)printf("Built using FCTX version %s.\n", FCT_VERSION_STR);
        status = -1;
        goto finally;
    }
    if ( fctkern__cl_is(nk, FCT_OPT_HELP) )
    {
        fctkern__write_help(nk, stdout);
        status = -1;
        goto finally;
    }
    if ( !fctkern__cl_parse_config_logger(nk) )
    {
        status = -1;
        goto finally;
    }
    status =1;
    nk->cl_is_parsed =1;
finally:
    return status;
}



/* Parses the command line and sets up the framework. The argc and argv
should be directly from the program's main. */
static int
fctkern__init(fctkern_t *nk, int argc, const char *argv[])
{
    if ( argc == 0 && argv == NULL )
    {
        return 0;
    }
    memset(nk, 0, sizeof(fctkern_t));
    fct_clp__init(&(nk->cl_parser), NULL);
    fct_nlist__init(&(nk->logger_list));
    nk->lt_usr = NULL;  /* Supplied via 'install' mechanics. */
    nk->lt_sys = FCT_LOGGER_TYPES;
    fct_nlist__init2(&(nk->prefix_list), 0);
    fct_nlist__init2(&(nk->ts_list), 0);
    nk->cl_is_parsed =0;
    /* Save a copy of the arguments. We do a delay parse of the command
    line arguments in order to allow the client code to optionally configure
    the command line parser.*/
    nk->cl_argc = argc;
    nk->cl_argv = argv;
    fct_namespace_init(&(nk->ns));
    return 1;
}


/* Takes OWNERSHIP of the test suite after we have finished executing
its contents. This way we can build up all kinds of summaries at the end
of a run. */
static void
fctkern__add_ts(fctkern_t *nk, fct_ts_t *ts)
{
    FCT_ASSERT( nk != NULL );
    FCT_ASSERT( ts != NULL );
    fct_nlist__append(&(nk->ts_list), ts);
}


/* Returns FCT_TRUE if the supplied test_name passes the filters set on
this test suite. If there are no filters, we return FCT_TRUE always. */
static nbool_t
fctkern__pass_filter(fctkern_t *nk, char const *test_name)
{
    size_t prefix_i =0;
    size_t prefix_list_size =0;
    FCT_ASSERT( nk != NULL && "invalid arg");
    FCT_ASSERT( test_name != NULL );
    FCT_ASSERT( strlen(test_name) > 0 );
    prefix_list_size = fctkern__filter_cnt(nk);
    /* If there is no filter list, then we return FCT_TRUE always. */
    if ( prefix_list_size == 0 )
    {
        return FCT_TRUE;
    }
    /* Iterate through the prefix filter list, and see if we have
    anything that does not pass. All we require is ONE item that
    passes the test in order for us to succeed here. */
    for ( prefix_i = 0; prefix_i != prefix_list_size; ++prefix_i )
    {
        char const *prefix = (char const*)fct_nlist__at(
                                 &(nk->prefix_list), prefix_i
                             );
        nbool_t pass = fct_filter_pass(prefix, test_name);
        if ( pass )
        {
            return FCT_TRUE;
        }
    }
    /* Otherwise, we never managed to find a prefix that satisfied the
    supplied test name. Therefore we have failed to pass to the filter
    list test. */
    return FCT_FALSE;
}


/* Returns the number of tests that were performed. */
static size_t
fctkern__tst_cnt(fctkern_t const *nk)
{
    size_t tally =0;
    FCT_ASSERT( nk != NULL );
    FCT_NLIST_FOREACH_BGN(fct_ts_t *, ts, &(nk->ts_list))
    {
        tally += fct_ts__tst_cnt(ts);
    }
    FCT_NLIST_FOREACH_END();
    return tally;
}


/* Returns the number of tests that passed. */
static size_t
fctkern__tst_cnt_passed(fctkern_t const *nk)
{
    size_t tally =0;
    FCT_ASSERT( nk != NULL );

    FCT_NLIST_FOREACH_BGN(fct_ts_t*, ts, &(nk->ts_list))
    {
        tally += fct_ts__tst_cnt_passed(ts);
    }
    FCT_NLIST_FOREACH_END();

    return tally;
}


/* Returns the number of tests that failed. */
#define fctkern__tst_cnt_failed(nk) \
    (fctkern__tst_cnt(nk) - fctkern__tst_cnt_passed(nk))


/* Returns the number of checks made throughout the entire test. */
#if defined(FCT_USE_TEST_COUNT)
static size_t
fctkern__chk_cnt(fctkern_t const *nk)
{
    size_t tally =0;
    FCT_ASSERT( nk != NULL );

    FCT_NLIST_FOREACH_BGN(fct_ts_t *, ts, &(nk->ts_list))
    {
        tally += fct_ts__chk_cnt(ts);
    }
    FCT_NLIST_FOREACH_END();
    return tally;
}
#endif /* FCT_USE_TEST_COUNT */


/* Indicates the very end of all the tests. */
#define fctkern__end(nk) /* unused */


static void
fctkern__log_suite_start(fctkern_t *nk, fct_ts_t const *ts)
{
    FCT_ASSERT( nk != NULL );
    FCT_ASSERT( ts != NULL );
    FCT_NLIST_FOREACH_BGN(fct_logger_i*, logger, &(nk->logger_list))
    {
        fct_logger__on_test_suite_start(logger, ts);
    }
    FCT_NLIST_FOREACH_END();
}


static void
fctkern__log_suite_end(fctkern_t *nk, fct_ts_t const *ts)
{
    FCT_ASSERT( nk != NULL );
    FCT_ASSERT( ts != NULL );
    FCT_NLIST_FOREACH_BGN(fct_logger_i*, logger, &(nk->logger_list))
    {
        fct_logger__on_test_suite_end(logger, ts);
    }
    FCT_NLIST_FOREACH_END();
}


static void
fctkern__log_suite_skip(fctkern_t *nk, char const *condition, char const *name)
{
    if ( nk == NULL )
    {
        return;
    }
    FCT_NLIST_FOREACH_BGN(fct_logger_i*, logger, &(nk->logger_list))
    {
        fct_logger__on_test_suite_skip(logger, condition, name);
    }
    FCT_NLIST_FOREACH_END();
}


static void
fctkern__log_test_skip(fctkern_t *nk, char const *condition, char const *name)
{
    FCT_NLIST_FOREACH_BGN(fct_logger_i*, logger, &(nk->logger_list))
    {
        fct_logger__on_test_skip(logger, condition, name);
    }
    FCT_NLIST_FOREACH_END();
}


/* Use this for displaying information about a "Check" (i.e.
a condition). */
static void
fctkern__log_chk(fctkern_t *nk, fctchk_t const *chk)
{
    FCT_ASSERT( nk != NULL );
    FCT_ASSERT( chk != NULL );
    FCT_NLIST_FOREACH_BGN(fct_logger_i*, logger, &(nk->logger_list))
    {
        fct_logger__on_chk(logger, chk);
    }
    FCT_NLIST_FOREACH_END();
}


/* Use this for displaying warning messages. */
static void
fctkern__log_warn(fctkern_t *nk, char const *warn)
{
    FCT_ASSERT( nk != NULL );
    FCT_ASSERT( warn != NULL );
    FCT_NLIST_FOREACH_BGN(fct_logger_i*, logger, &(nk->logger_list))
    {
        fct_logger__on_warn(logger, warn);
    }
    FCT_NLIST_FOREACH_END();
}


/* Called whenever a test is started. */
static void
fctkern__log_test_start(fctkern_t *nk, fct_test_t const *test)
{
    FCT_ASSERT( nk != NULL );
    FCT_ASSERT( test != NULL );
    FCT_NLIST_FOREACH_BGN(fct_logger_i*, logger, &(nk->logger_list))
    {
        fct_logger__on_test_start(logger, test);
    }
    FCT_NLIST_FOREACH_END();
}


static void
fctkern__log_test_end(fctkern_t *nk, fct_test_t *test)
{
    FCT_ASSERT( nk != NULL );
    FCT_ASSERT( test != NULL );
    FCT_NLIST_FOREACH_BGN(fct_logger_i*, logger, &(nk->logger_list))
    {
        fct_logger__on_test_end(logger, test);
    }
    FCT_NLIST_FOREACH_END();
}


#define fctkern__log_start(_NK_) \
   {\
       FCT_NLIST_FOREACH_BGN(fct_logger_i*, logger, &((_NK_)->logger_list))\
       {\
          fct_logger__on_fctx_start(logger, (_NK_));\
       }\
       FCT_NLIST_FOREACH_END();\
   }


#define fctkern__log_end(_NK_) \
    {\
       FCT_NLIST_FOREACH_BGN(fct_logger_i*, logger, &((_NK_)->logger_list))\
       {\
          fct_logger__on_fctx_end(logger, (_NK_));\
       }\
       FCT_NLIST_FOREACH_END();\
    }




/*
-----------------------------------------------------------
LOGGER INTERFACE

Defines an interface to a logging system. A logger
must define the following functions in order to hook
into the logging system.

See the "Standard Logger" and "Minimal Logger" as examples
of the implementation.
-----------------------------------------------------------
*/

/* Common event argument. The values of the each event may or may not be
defined depending on the event in question. */
struct _fct_logger_evt_t
{
    fctkern_t const *kern;
    fctchk_t const *chk;
    fct_test_t const *test;
    fct_ts_t const *ts;
    char const *msg;
    char const *cndtn;
    char const *name;
};


typedef struct _fct_logger_i_vtable_t
{
    /* 1
     * Fired when an "fct_chk*" (check) function is completed. The event
     * will contain a reference to the "chk" object created.
     * */
    void (*on_chk)(fct_logger_i *logger, fct_logger_evt_t const *e);

    /* 2
     * Fired when a test starts and before any checks are made. The
     * event will have its "test" object set. */
    void (*on_test_start)(
        fct_logger_i *logger,
        fct_logger_evt_t const *e
    );
    /* 3 */
    void (*on_test_end)(
        fct_logger_i *logger,
        fct_logger_evt_t const *e
    );
    /* 4 */
    void (*on_test_suite_start)(
        fct_logger_i *logger,
        fct_logger_evt_t const *e
    );
    /* 5 */
    void (*on_test_suite_end)(
        fct_logger_i *logger,
        fct_logger_evt_t const *e
    );
    /* 6 */
    void (*on_fctx_start)(
        fct_logger_i *logger,
        fct_logger_evt_t const *e
    );
    /* 7 */
    void (*on_fctx_end)(
        fct_logger_i *logger,
        fct_logger_evt_t const *e
    );
    /* 8
    Called when the logger object must "clean up". */
    void (*on_delete)(
        fct_logger_i *logger,
        fct_logger_evt_t const *e
    );
    /* 9 */
    void (*on_warn)(
        fct_logger_i *logger,
        fct_logger_evt_t const *e
    );
    /* -- new in 1.2 -- */
    /* 10 */
    void (*on_test_suite_skip)(
        fct_logger_i *logger,
        fct_logger_evt_t const *e
    );
    /* 11 */
    void (*on_test_skip)(
        fct_logger_i *logger,
        fct_logger_evt_t const *e
    );
} fct_logger_i_vtable_t;

#define _fct_logger_head \
    fct_logger_i_vtable_t vtable; \
    fct_logger_evt_t evt

struct _fct_logger_i
{
    _fct_logger_head;
};


static void
fct_logger__stub(fct_logger_i *l, fct_logger_evt_t const *e)
{
    fct_unused(l);
    fct_unused(e);
}


static fct_logger_i_vtable_t fct_logger_default_vtable =
{
    fct_logger__stub,   /* 1.  on_chk */
    fct_logger__stub,   /* 2.  on_test_start */
    fct_logger__stub,   /* 3.  on_test_end */
    fct_logger__stub,   /* 4.  on_test_suite_start */
    fct_logger__stub,   /* 5.  on_test_suite_end */
    fct_logger__stub,   /* 6.  on_fctx_start */
    fct_logger__stub,   /* 7.  on_fctx_end */
    fct_logger__stub,   /* 8.  on_delete */
    fct_logger__stub,   /* 9.  on_warn */
    fct_logger__stub,   /* 10. on_test_suite_skip */
    fct_logger__stub,   /* 11. on_test_skip */
};


/* Initializes the elements of a logger interface so they are at their
standard values. */
static void
fct_logger__init(fct_logger_i *logger)
{
    FCT_ASSERT( logger != NULL );
    memcpy(
        &(logger->vtable),
        &fct_logger_default_vtable,
        sizeof(fct_logger_i_vtable_t)
    );
    memset(&(logger->evt),0, sizeof(fct_logger_evt_t));
}

static void
fct_logger__del(fct_logger_i *logger)
{
    if ( logger )
    {
        logger->vtable.on_delete(logger, &(logger->evt));
    }
}


static void
fct_logger__on_test_start(fct_logger_i *logger, fct_test_t const *test)
{
    logger->evt.test = test;
    logger->vtable.on_test_start(logger, &(logger->evt));
}


static void
fct_logger__on_test_end(fct_logger_i *logger, fct_test_t *test)
{
    logger->evt.test = test;
    logger->vtable.on_test_end(logger, &(logger->evt));
}


static void
fct_logger__on_test_suite_start(fct_logger_i *logger, fct_ts_t const *ts)
{
    logger->evt.ts = ts;
    logger->vtable.on_test_suite_start(logger, &(logger->evt));
}


static void
fct_logger__on_test_suite_end(fct_logger_i *logger, fct_ts_t const *ts)
{
    logger->evt.ts = ts;
    logger->vtable.on_test_suite_end(logger, &(logger->evt));
}


static void
fct_logger__on_test_suite_skip(
    fct_logger_i *logger,
    char const *condition,
    char const *name
)
{
    logger->evt.cndtn = condition;
    logger->evt.name = name;
    logger->vtable.on_test_suite_skip(logger, &(logger->evt));
}


static void
fct_logger__on_test_skip(
    fct_logger_i *logger,
    char const *condition,
    char const *name
)
{
    logger->evt.cndtn = condition;
    logger->evt.name = name;
    logger->vtable.on_test_skip(logger, &(logger->evt));
}


static void
fct_logger__on_chk(fct_logger_i *logger, fctchk_t const *chk)
{
    logger->evt.chk = chk;
    logger->vtable.on_chk(logger, &(logger->evt));
}

/* When we start all our tests. */
#define fct_logger__on_fctx_start(LOGGER, KERN) \
   (LOGGER)->evt.kern = (KERN);\
   (LOGGER)->vtable.on_fctx_start((LOGGER), &((LOGGER)->evt));


/* When we have reached the end of ALL of our testing. */
#define fct_logger__on_fctx_end(LOGGER, KERN) \
    (LOGGER)->evt.kern = (KERN);\
    (LOGGER)->vtable.on_fctx_end((LOGGER), &((LOGGER)->evt));


static void
fct_logger__on_warn(fct_logger_i *logger, char const *msg)
{
    logger->evt.msg = msg;
    logger->vtable.on_warn(logger, &(logger->evt));
}


/* Commmon routine to record strings representing failures. The
chk should be a failure before we call this, and the list is a list
of char*'s that will eventually be free'd by the logger. */
static void
fct_logger_record_failure(fctchk_t const* chk, fct_nlist_t* fail_list)
{
    /* For now we will truncate the string to some set amount, later
    we can work out a dynamic string object. */
    char *str = (char*)malloc(sizeof(char)*FCT_MAX_LOG_LINE);
    FCT_ASSERT( str != NULL );
    fct_snprintf(
        str,
        FCT_MAX_LOG_LINE,
        "%s(%d):\n    %s",
        fctchk__file(chk),
        fctchk__lineno(chk),
        fctchk__msg(chk)
    );
    /* Append it to the listing ... */
    fct_nlist__append(fail_list, (void*)str);
}


/* Another common routine, to print the failures at the end of a run. */
static void
fct_logger_print_failures(fct_nlist_t const *fail_list)
{
    puts(
        "\n----------------------------------------------------------------------------\n"
    );
    puts("FAILED TESTS\n\n");
    FCT_NLIST_FOREACH_BGN(char *, cndtn_str, fail_list)
    {
        printf("%s\n", cndtn_str);
    }
    FCT_NLIST_FOREACH_END();

    puts("\n");
}




/*
-----------------------------------------------------------
MINIMAL LOGGER
-----------------------------------------------------------

At the moment the MINIMAL LOGGER is currently disabled. Hope
to bring it back online soon. The only reason it is
disabled is that we don't currently have the ability to specify
loggers.
*/


/* Minimal logger, reports the minimum amount of information needed
to determine "something is happening". */
struct _fct_minimal_logger_t
{
    _fct_logger_head;
    /* A list of char*'s that needs to be cleaned up. */
    fct_nlist_t failed_cndtns_list;
};


static void
fct_minimal_logger__on_chk(
    fct_logger_i *self_,
    fct_logger_evt_t const *e
)
{
    fct_minimal_logger_t *self = (fct_minimal_logger_t*)self_;
    if ( fctchk__is_pass(e->chk) )
    {
        fputs(".", stdout);
    }
    else
    {
        fputs("x", stdout);
        fct_logger_record_failure(e->chk, &(self->failed_cndtns_list));

    }
}

static void
fct_minimal_logger__on_fctx_end(
    fct_logger_i *self_,
    fct_logger_evt_t const *e
)
{
    fct_minimal_logger_t *self = (fct_minimal_logger_t*)self_;
    fct_unused(e);
    if ( fct_nlist__size(&(self->failed_cndtns_list)) >0 )
    {
        fct_logger_print_failures(&(self->failed_cndtns_list));
    }
}


static void
fct_minimal_logger__on_delete(
    fct_logger_i *self_,
    fct_logger_evt_t const *e
)
{
    fct_minimal_logger_t *self = (fct_minimal_logger_t*)self_;
    fct_unused(e);
    fct_nlist__final(&(self->failed_cndtns_list), free);
    free(self);

}


fct_logger_i*
fct_minimal_logger_new(void)
{
    fct_minimal_logger_t *self = (fct_minimal_logger_t*)\
                                 calloc(1,sizeof(fct_minimal_logger_t));
    if ( self == NULL )
    {
        return NULL;
    }
    fct_logger__init((fct_logger_i*)self);
    self->vtable.on_chk = fct_minimal_logger__on_chk;
    self->vtable.on_fctx_end = fct_minimal_logger__on_fctx_end;
    self->vtable.on_delete = fct_minimal_logger__on_delete;
    fct_nlist__init2(&(self->failed_cndtns_list), 0);
    return (fct_logger_i*)self;
}


/*
-----------------------------------------------------------
STANDARD LOGGER
-----------------------------------------------------------
*/

struct _fct_standard_logger_t
{
    _fct_logger_head;

    /* Start time. For now we use the low-accuracy time_t version. */
    fct_timer_t timer;

    /* A list of char*'s that needs to be cleaned up. */
    fct_nlist_t failed_cndtns_list;
};


#define FCT_STANDARD_LOGGER_MAX_LINE 68


/* When a failure occurrs, we will record the details so we can display
them when the log "finishes" up. */
static void
fct_standard_logger__on_chk(
    fct_logger_i *logger_,
    fct_logger_evt_t const *e
)
{
    fct_standard_logger_t *logger = (fct_standard_logger_t*)logger_;
    /* Only record failures. */
    if ( !fctchk__is_pass(e->chk) )
    {
        fct_logger_record_failure(e->chk, &(logger->failed_cndtns_list));
    }
}


static void
fct_standard_logger__on_test_skip(
    fct_logger_i* logger_,
    fct_logger_evt_t const *e
)
{
    char const *condition = e->cndtn;
    char const *name = e->name;
    char msg[256] = {'\0'};
    fct_unused(logger_);
    fct_unused(condition);
    fct_snprintf(msg, sizeof(msg), "%s (%s)", name, condition);
    msg[sizeof(msg)-1] = '\0';
    fct_dotted_line_start(FCT_STANDARD_LOGGER_MAX_LINE, msg);
    fct_dotted_line_end("- SKIP -");
}


static void
fct_standard_logger__on_test_start(
    fct_logger_i *logger_,
    fct_logger_evt_t const *e
)
{
    fct_unused(logger_);
    fct_dotted_line_start(
        FCT_STANDARD_LOGGER_MAX_LINE,
        fct_test__name(e->test)
    );
}


static void
fct_standard_logger__on_test_end(
    fct_logger_i *logger_,
    fct_logger_evt_t const *e
)
{
    nbool_t is_pass;
    fct_unused(logger_);
    is_pass = fct_test__is_pass(e->test);
    fct_dotted_line_end((is_pass) ? "PASS" : "FAIL ***" );
}


static void
fct_standard_logger__on_fctx_start(
    fct_logger_i *logger_,
    fct_logger_evt_t const *e
)
{
    fct_standard_logger_t *logger = (fct_standard_logger_t*)logger_;
    fct_unused(e);
    fct_timer__start(&(logger->timer));
}


static void
fct_standard_logger__on_fctx_end(
    fct_logger_i *logger_,
    fct_logger_evt_t const *e
)
{
    fct_standard_logger_t *logger = (fct_standard_logger_t*)logger_;
    nbool_t is_success =1;
    double elasped_time =0;
    size_t num_tests =0;
    size_t num_passed =0;

    fct_timer__stop(&(logger->timer));

    is_success = fct_nlist__size(&(logger->failed_cndtns_list)) ==0;

    if (  !is_success )
    {
        fct_logger_print_failures(&(logger->failed_cndtns_list));
    }
    puts(
        "\n----------------------------------------------------------------------------\n"
    );
    num_tests = fctkern__tst_cnt(e->kern);
    num_passed = fctkern__tst_cnt_passed(e->kern);
    printf(
        "%s (%lu/%lu tests",
        (is_success) ? "PASSED" : "FAILED",
        (unsigned long) num_passed,
        (unsigned long) num_tests
    );
    elasped_time = fct_timer__duration(&(logger->timer));
    if ( elasped_time > 0.0000001 )
    {
        printf(" in %.6fs)\n", elasped_time);
    }
    else
    {
        /* Don't bother displaying the time to execute. */
        puts(")\n");
    }
}


static void
fct_standard_logger__on_delete(
    fct_logger_i *logger_,
    fct_logger_evt_t const *e
)
{
    fct_standard_logger_t *logger = (fct_standard_logger_t*)logger_;
    fct_unused(e);
    fct_nlist__final(&(logger->failed_cndtns_list), free);
    free(logger);
    logger_ =NULL;
}


static void
fct_standard_logger__on_warn(
    fct_logger_i* logger_,
    fct_logger_evt_t const *e
)
{
    fct_unused(logger_);
    (void)printf("WARNING: %s", e->msg);
}


fct_logger_i*
fct_standard_logger_new(void)
{
    fct_standard_logger_t *logger = (fct_standard_logger_t *)calloc(
                                        1, sizeof(fct_standard_logger_t)
                                    );
    if ( logger == NULL )
    {
        return NULL;
    }
    fct_logger__init((fct_logger_i*)logger);
    logger->vtable.on_chk = fct_standard_logger__on_chk;
    logger->vtable.on_test_start = fct_standard_logger__on_test_start;
    logger->vtable.on_test_end = fct_standard_logger__on_test_end;
    logger->vtable.on_fctx_start = fct_standard_logger__on_fctx_start;
    logger->vtable.on_fctx_end = fct_standard_logger__on_fctx_end;
    logger->vtable.on_delete = fct_standard_logger__on_delete;
    logger->vtable.on_warn = fct_standard_logger__on_warn;
    logger->vtable.on_test_skip = fct_standard_logger__on_test_skip;
    fct_nlist__init2(&(logger->failed_cndtns_list), 0);
    fct_timer__init(&(logger->timer));
    return (fct_logger_i*)logger;
}


/*
-----------------------------------------------------------
JUNIT LOGGER
-----------------------------------------------------------
*/


/* JUnit logger */
struct _fct_junit_logger_t
{
    _fct_logger_head;
};


static void
fct_junit_logger__on_test_suite_start(
    fct_logger_i *l,
    fct_logger_evt_t const *e
)
{
    fct_unused(l);
    fct_unused(e);
    FCT_SWITCH_STDOUT_TO_BUFFER();
    FCT_SWITCH_STDERR_TO_BUFFER();
}


static void
fct_junit_logger__on_test_suite_end(
    fct_logger_i *logger_,
    fct_logger_evt_t const *e
)
{
    fct_ts_t const *ts = e->ts; /* Test Suite */
    nbool_t is_pass;
    double elasped_time = 0;
    char std_buffer[1024];
    int read_length;
    int first_out_line;

    fct_unused(logger_);

    elasped_time = fct_ts__duration(ts);

    FCT_SWITCH_STDOUT_TO_STDOUT();
    FCT_SWITCH_STDERR_TO_STDERR();

    /* opening testsuite tag */
    printf("\t<testsuite errors=\"%lu\" failures=\"0\" tests=\"%lu\" "
           "name=\"%s\" time=\"%.4f\">\n",
           (unsigned long)   fct_ts__tst_cnt(ts)
           - fct_ts__tst_cnt_passed(ts),
           (unsigned long) fct_ts__tst_cnt(ts),
           fct_ts__name(ts),
           elasped_time);

    FCT_NLIST_FOREACH_BGN(fct_test_t*, test, &(ts->test_list))
    {
        is_pass = fct_test__is_pass(test);

        /* opening testcase tag */
        if (is_pass)
        {
            printf("\t\t<testcase name=\"%s\" time=\"%.3f\"",
                   fct_test__name(test),
                   fct_test__duration(test)
                  );
        }
        else
        {
            printf("\t\t<testcase name=\"%s\" time=\"%.3f\">\n",
                   fct_test__name(test),
                   fct_test__duration(test)
                  );
        }

        FCT_NLIST_FOREACH_BGN(fctchk_t*, chk, &(test->failed_chks))
        {
            /* error tag */
            printf("\t\t\t<error message=\"%s\" "
                   "type=\"fctx\">", chk->msg);
            printf("file:%s, line:%d", chk->file, chk->lineno);
            printf("</error>\n");
        }
        FCT_NLIST_FOREACH_END();

        /* closing testcase tag */
        if (is_pass)
        {
            printf(" />\n");
        }
        else
        {
            printf("\t\t</testcase>\n");
        }
    }
    FCT_NLIST_FOREACH_END();

    /* print the std streams */
    first_out_line = 1;
    printf("\t\t<system-out>\n\t\t\t<![CDATA[");
    while ( (read_length = _fct_read(fct_stdout_pipe[0], std_buffer, 1024)) > 0)
    {
        if (first_out_line)
        {
            printf("\n");
            first_out_line = 0;
        }
        printf("%.*s", read_length, std_buffer);
    }
    printf("]]>\n\t\t</system-out>\n");

    first_out_line = 1;
    printf("\t\t<system-err>\n\t\t\t<![CDATA[");
    while ((read_length = _fct_read(fct_stderr_pipe[0], std_buffer, 1024)) > 0)
    {
        if (first_out_line)
        {
            printf("\n");
            first_out_line = 0;
        }
        printf("%.*s", read_length, std_buffer);
    }
    printf("]]>\n\t\t</system-err>\n");

    /* closing testsuite tag */
    printf("\t</testsuite>\n");
}

static void
fct_junit_logger__on_fct_start(
    fct_logger_i *logger_,
    fct_logger_evt_t const *e
)
{
    fct_unused(logger_);
    fct_unused(e);
    printf("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
    printf("<testsuites>\n");
}

static void
fct_junit_logger__on_fctx_end(
    fct_logger_i *logger_,
    fct_logger_evt_t const *e
)
{
    fct_unused(logger_);
    fct_unused(e);
    printf("</testsuites>\n");
}

static void
fct_junit_logger__on_delete(
    fct_logger_i *logger_,
    fct_logger_evt_t const *e
)
{
    fct_junit_logger_t *logger = (fct_junit_logger_t*)logger_;
    fct_unused(e);
    free(logger);
    logger_ =NULL;
}


fct_junit_logger_t *
fct_junit_logger_new(void)
{
    fct_junit_logger_t *logger =
        (fct_junit_logger_t *)calloc(1, sizeof(fct_junit_logger_t));
    if ( logger == NULL )
    {
        return NULL;
    }
    fct_logger__init((fct_logger_i*)logger);
    logger->vtable.on_test_suite_start = fct_junit_logger__on_test_suite_start;
    logger->vtable.on_test_suite_end = fct_junit_logger__on_test_suite_end;
    logger->vtable.on_fctx_start = fct_junit_logger__on_fct_start;
    logger->vtable.on_fctx_end = fct_junit_logger__on_fctx_end;
    logger->vtable.on_delete = fct_junit_logger__on_delete;
    return logger;
}


/*
------------------------------------------------------------
MACRO MAGIC
------------------------------------------------------------
This is where the show begins!
*/

/* This macro invokes a bunch of functions that need to be referenced in
order to avoid  a "unreferenced local function has been removed" warning.
The logical acrobatics below try and make it appear to the compiler that
they are needed, but at runtime, only the cheap, first call is made. */
#define FCT_REFERENCE_FUNCS() \
    {\
        int check = 0 && fctstr_ieq(NULL, NULL);\
        if ( check ) {\
            _fct_cmt("not to be executed");\
            (void)_fct_chk_empty_str(NULL);\
            (void)_fct_chk_full_str(NULL);\
            (void)fct_test__start_timer(NULL);\
            (void)fct_test__stop_timer(NULL);\
            (void)fct_ts_new(NULL);\
            (void)fct_ts__test_begin(NULL);\
            (void)fct_ts__add_test(NULL, NULL);\
            (void)fct_ts__test_end(NULL);\
            (void)fct_ts__inc_total_test_num(NULL);\
            (void)fct_ts__make_abort_test(NULL);\
            (void)fct_ts__setup_abort(NULL);\
            (void)fct_ts__setup_end(NULL);\
            (void)fct_ts__teardown_end(NULL);\
            (void)fct_ts__cnt_end(NULL);\
            (void)fct_ts__is_test_cnt(NULL, 0);\
            (void)fct_xchk_fn(0, "");\
            (void)fct_xchk2_fn(NULL, 0, "");\
            (void)fctkern__cl_parse(NULL);\
            (void)fctkern__add_ts(NULL, NULL);\
            (void)fctkern__pass_filter(NULL, NULL);\
            (void)fctkern__log_suite_start(NULL, NULL);\
            (void)fctkern__log_suite_end(NULL, NULL);\
            (void)fctkern__log_test_skip(NULL, NULL, NULL);\
            (void)fctkern__log_test_start(NULL, NULL);\
            (void)fctkern__log_test_end(NULL, NULL);\
            (void)fctstr_endswith(NULL,NULL);\
            (void)fctstr_iendswith(NULL,NULL);\
            (void)fctstr_ieq(NULL,NULL);\
            (void)fctstr_incl(NULL, NULL);\
            (void)fctstr_iincl(NULL, NULL);\
            (void)fctstr_iendswith(NULL,NULL);\
            (void)fctstr_istartswith(NULL,NULL);\
            (void)fctstr_clone_lower(NULL);\
            (void)fctstr_startswith(NULL,NULL);\
            (void)fctkern__init(NULL, 0, NULL);\
            (void)fctkern__cl_is(NULL, "");\
            (void)fctkern__cl_val2(NULL, NULL, NULL);\
            fctkern__log_suite_skip(NULL, NULL, NULL);\
            (void)fct_clp__is_param(NULL,NULL);\
	    _fct_cmt("should never construct an object");\
            (void)fct_test_new(NULL);\
            (void)fct_ts__chk_cnt(NULL);\
        }\
    }


#define FCT_INIT(_ARGC_, _ARGV_)                                            \
   fctkern_t  fctkern__;                                                    \
   fctkern_t* fctkern_ptr__ = &fctkern__;                                   \
   FCT_REFERENCE_FUNCS();                                                   \
   if ( !fctkern__init(fctkern_ptr__, (_ARGC_), (const char **)(_ARGV_)) ) {\
        (void)fprintf(                                                      \
            stderr, "FATAL ERROR: Unable to initialize FCTX Kernel."        \
        );                                                                  \
        exit(EXIT_FAILURE);                                                 \
   }                                                                        \


#define FCT_FINAL()                                                \
   fctkern_ptr__->ns.num_total_failed = fctkern__tst_cnt_failed(   \
            (fctkern_ptr__)                                        \
           );                                                      \
   fctkern__log_end(fctkern_ptr__);                                \
   fctkern__end(fctkern_ptr__);                                    \
   fctkern__final(fctkern_ptr__);                                  \
   FCT_ASSERT( !((int)fctkern_ptr__->ns.num_total_failed < 0)      \
               && "or we got truncated!");                         \
   if ( fctkern_ptr__->ns.num_total_failed ==                      \
        fctkern_ptr__->num_expected_failures) {                    \
       fctkern_ptr__->ns.num_total_failed = 0;                     \
   }                                                               \



#define FCT_NUM_FAILED()       \
    fctkern_ptr__->ns.num_total_failed \



/* Typically used internally only, this mentions to FCTX that you EXPECT
to _NUM_FAILS_. If you the expected matches the actual, a 0 value is returned
from the program. */
#define FCT_EXPECTED_FAILURES(_NUM_FAILS_) \
    ((fctkern_ptr__->num_expected_failures = (_NUM_FAILS_)))


#define FCT_BGN_FN(_FNNAME_)            \
    int _FNNAME_(int argc, char* argv[])\
    {                                   \
        FCT_INIT(argc, argv)

#define FCT_END_FN() FCT_END()

/* This defines our start. The fctkern__ is a kernal object
that lives throughout the lifetime of our program. The
fctkern_ptr__ makes it easier to abstract out macros.  */
#define FCT_BGN() FCT_BGN_FN(main)


/* Silence Intel complaints about unspecified operand order in user's code */
#ifndef __INTEL_COMPILER
# define FCT_END_WARNINGFIX_BGN
# define FCT_END_WARNINGFIX_END
#else
# define FCT_END_WARNINGFIX_BGN _Pragma("warning(push,disable:981)");
# define FCT_END_WARNINGFIX_END _Pragma("warning(pop)");
#endif

/* Ends the test suite by returning the number failed. The "chk_cnt" call is
made in order allow strict compilers to pass when it encounters unreferenced
functions. */
#define FCT_END()                   \
   {                                \
      FCT_END_WARNINGFIX_BGN        \
      FCT_FINAL();                  \
      return (int) FCT_NUM_FAILED();\
      FCT_END_WARNINGFIX_END        \
   }\
}

#define fctlog_install(_CUST_LOGGER_LIST_) \
    fctkern_ptr__->lt_usr = (_CUST_LOGGER_LIST_)

/* Re-parses the command line options with the addition of user defined
options. */
#define fctcl_install(_CLO_INIT_) \
    {\
        fctkern_ptr__->cl_user_opts = (_CLO_INIT_);\
        _fct_cmt("Delay parse in order to allow for user customization.");\
        if ( !fctkern__cl_is_parsed((fctkern_ptr__)) ) {\
              int status = fctkern__cl_parse((fctkern_ptr__));\
              _fct_cmt("Need to parse command line before we start logger.");\
              fctkern__log_start((fctkern_ptr__));\
              switch( status ) {\
              case -1:\
              case 0:\
                  fctkern__final(fctkern_ptr__);\
                  exit( (status == 0) ? (EXIT_FAILURE) : (EXIT_SUCCESS) );\
                  break;\
              default:\
                  fct_pass();\
              }\
          }\
    }


#define fctcl_is(_OPT_STR_) (fctkern__cl_is(fctkern_ptr__, (_OPT_STR_)))

#define fctcl_val(_OPT_STR_) (fctcl_val2((_OPT_STR_), NULL))

#define fctcl_val2(_OPT_STR_, _DEF_STR_) \
   (fctkern__cl_val2(fctkern_ptr__, (_OPT_STR_), (_DEF_STR_)))


/* We delay the first parse of the command line until we get the first
test fixture. This allows the user to possibly add their own parse
specification. */
#define FCT_FIXTURE_SUITE_BGN(_NAME_) \
   {\
      fctkern_ptr__->ns.ts_curr = fct_ts_new( #_NAME_ );\
      _fct_cmt("Delay parse in order to allow for user customization.");\
      if ( !fctkern__cl_is_parsed((fctkern_ptr__)) ) {\
          int status = fctkern__cl_parse((fctkern_ptr__));\
          _fct_cmt("Need to parse command line before we start logger.");\
          fctkern__log_start((fctkern_ptr__));\
          switch( status ) {\
          case -1:\
          case 0:\
              fct_ts__del((fctkern_ptr__->ns.ts_curr));\
              fctkern__final(fctkern_ptr__);\
              exit( (status == 0) ? (EXIT_FAILURE) : (EXIT_SUCCESS) );\
              break;\
          default:\
              fct_pass();\
          }\
      }\
      if ( fctkern_ptr__->ns.ts_curr == NULL ) {\
         fctkern__log_warn((fctkern_ptr__), "out of memory");\
      }\
      else\
      {\
         fctkern__log_suite_start((fctkern_ptr__), fctkern_ptr__->ns.ts_curr);\
         for (;;)\
         {\
             fctkern_ptr__->ns.test_num = -1;\
             if ( fct_ts__is_ending_mode(fctkern_ptr__->ns.ts_curr) \
                  || fct_ts__is_abort_mode(fctkern_ptr__->ns.ts_curr) )\
             {\
               _fct_cmt("flag the test suite as complete.");\
               fct_ts__end(fctkern_ptr__->ns.ts_curr);\
               break;\
             }



/*  Closes off a "Fixture" test suite. */
#define FCT_FIXTURE_SUITE_END() \
             if ( fct_ts__is_cnt_mode(fctkern_ptr__->ns.ts_curr) )\
             {\
                fct_ts__cnt_end(fctkern_ptr__->ns.ts_curr);\
             }\
          }\
          fctkern__add_ts((fctkern_ptr__), fctkern_ptr__->ns.ts_curr);\
          fctkern__log_suite_end((fctkern_ptr__), fctkern_ptr__->ns.ts_curr);\
          fct_ts__end(fctkern_ptr__->ns.ts_curr);\
          fctkern_ptr__->ns.ts_curr = NULL;\
          }\
      }

#define FCT_FIXTURE_SUITE_BGN_IF(_CONDITION_, _NAME_) \
    fctkern_ptr__->ns.ts_is_skip_suite = !(_CONDITION_);\
    fctkern_ptr__->ns.ts_skip_cndtn = #_CONDITION_;\
    if ( fctkern_ptr__->ns.ts_is_skip_suite ) {\
       fctkern__log_suite_skip((fctkern_ptr__), #_CONDITION_, #_NAME_);\
    }\
    FCT_FIXTURE_SUITE_BGN(_NAME_);

#define FCT_FIXTURE_SUITE_END_IF() \
    FCT_FIXTURE_SUITE_END();\
    fctkern_ptr__->ns.ts_is_skip_suite =0;\
    fctkern_ptr__->ns.ts_skip_cndtn =NULL;\

#define FCT_SETUP_BGN()\
   if ( fct_ts__is_setup_mode(fctkern_ptr__->ns.ts_curr) ) {

#define FCT_SETUP_END() \
   fct_ts__setup_end(fctkern_ptr__->ns.ts_curr); }

#define FCT_TEARDOWN_BGN() \
   if ( fct_ts__is_teardown_mode(fctkern_ptr__->ns.ts_curr) ) {\

#define FCT_TEARDOWN_END() \
   fct_ts__teardown_end(fctkern_ptr__->ns.ts_curr); \
   continue; \
   }

/* Lets you create a test suite, where maybe you don't want a fixture. We
do it by 'stubbing' out the setup/teardown logic. */
#define FCT_SUITE_BGN(Name) \
   FCT_FIXTURE_SUITE_BGN(Name) {\
   FCT_SETUP_BGN() {_fct_cmt("stubbed"); } FCT_SETUP_END()\
   FCT_TEARDOWN_BGN() {_fct_cmt("stubbed");} FCT_TEARDOWN_END()\

#define FCT_SUITE_END() } FCT_FIXTURE_SUITE_END()

#define FCT_SUITE_BGN_IF(_CONDITION_, _NAME_) \
    FCT_FIXTURE_SUITE_BGN_IF(_CONDITION_, (_NAME_)) {\
    FCT_SETUP_BGN() {_fct_cmt("stubbed"); } FCT_SETUP_END()\
    FCT_TEARDOWN_BGN() {_fct_cmt("stubbed");} FCT_TEARDOWN_END()\

#define FCT_SUITE_END_IF() } FCT_FIXTURE_SUITE_END_IF()

typedef enum
{
    FCT_TEST_END_FLAG_Default    = 0x0000,
    FCT_TEST_END_FLAG_ClearFail  = 0x0001
} FCT_TEST_END_FLAG;


#define FCT_TEST_BGN_IF(_CONDITION_, _NAME_) { \
    fctkern_ptr__->ns.test_is_skip = !(_CONDITION_);\
    fctkern_ptr__->ns.test_skip_cndtn = #_CONDITION_;\
    FCT_TEST_BGN(_NAME_) {\

#define FCT_TEST_END_IF() \
    } FCT_TEST_END();\
    fctkern_ptr__->ns.test_is_skip = 0;\
    fctkern_ptr__->ns.test_skip_cndtn = NULL;\
    }


/* Depending on whether or not we are counting the tests, we will have to
first determine if the test is the "current" count. Then we have to determine
if we can pass the filter. Finally we will execute everything so that when a
check fails, we can "break" out to the end of the test. And in between all
that we do a memory check and fail a test if we can't build a fct_test
object (should be rare). */
#define FCT_TEST_BGN(_NAME_) \
         {\
            fctkern_ptr__->ns.curr_test_name = #_NAME_;\
            ++(fctkern_ptr__->ns.test_num);\
            if ( fct_ts__is_cnt_mode(fctkern_ptr__->ns.ts_curr) )\
            {\
               fct_ts__inc_total_test_num(fctkern_ptr__->ns.ts_curr);\
            }\
            else if ( fct_ts__is_test_mode(fctkern_ptr__->ns.ts_curr) \
                      && fct_ts__is_test_cnt(fctkern_ptr__->ns.ts_curr, fctkern_ptr__->ns.test_num) )\
            {\
               fct_ts__test_begin(fctkern_ptr__->ns.ts_curr);\
               if ( fctkern__pass_filter(fctkern_ptr__,  fctkern_ptr__->ns.curr_test_name ) )\
               {\
                  fctkern_ptr__->ns.curr_test = fct_test_new( fctkern_ptr__->ns.curr_test_name );\
                  if ( fctkern_ptr__->ns.curr_test  == NULL ) {\
                    fctkern__log_warn(fctkern_ptr__, "out of memory");\
                  } else if ( fctkern_ptr__->ns.ts_is_skip_suite \
                              || fctkern_ptr__->ns.test_is_skip ) {\
                       fct_ts__test_begin(fctkern_ptr__->ns.ts_curr);\
                       fctkern__log_test_skip(\
                            fctkern_ptr__,\
                            fctkern_ptr__->ns.curr_test_name,\
                            (fctkern_ptr__->ns.test_is_skip) ?\
                                (fctkern_ptr__->ns.test_skip_cndtn) :\
                                (fctkern_ptr__->ns.ts_skip_cndtn)\
                       );\
                       fct_ts__test_end(fctkern_ptr__->ns.ts_curr);\
                       continue;\
                 } else {\
                      fctkern__log_test_start(fctkern_ptr__, fctkern_ptr__->ns.curr_test);\
                      fct_test__start_timer(fctkern_ptr__->ns.curr_test);\
                      for (;;) \
                      {




#define FCT_TEST_END() \
                         break;\
                      }\
                      fct_test__stop_timer(fctkern_ptr__->ns.curr_test);\
                 }\
                 fct_ts__add_test(fctkern_ptr__->ns.ts_curr, fctkern_ptr__->ns.curr_test);\
                 fctkern__log_test_end(fctkern_ptr__, fctkern_ptr__->ns.curr_test);\
               }\
               fct_ts__test_end(fctkern_ptr__->ns.ts_curr);\
               continue;\
            }\
         }\



/*
---------------------------------------------------------
CHECKING MACROS
----------------------------------------------------------

The chk variants will continue on while the req variants will abort
a test if a chk condition fails. The req variants are useful when you
no longer want to keep checking conditions because a critical condition
is not being met. */


/* To support older compilers that do not have macro variable argument lists
we have to use a function. The macro manages to store away the line/file
location into a global before it runs this function, a trick I picked up from
the error handling in the APR library. The unfortunate thing is that we can
not carry forth the actual test through a "stringize" operation, but if you
wanted to do that you should use fct_chk. */

static int fct_xchk_lineno =0;
static char const *fct_xchk_file = NULL;
static fct_test_t *fct_xchk_test = NULL;
static fctkern_t *fct_xchk_kern =NULL;


static int
_fct_xchk_fn_varg(
    char const *condition,
    int is_pass,
    char const *format,
    va_list args
)
{
    fctchk_t *chk =NULL;
    chk = fctchk_new(
              is_pass,
              condition,
              fct_xchk_file,
              fct_xchk_lineno,
              format,
              args
          );
    if ( chk == NULL )
    {
        fctkern__log_warn(fct_xchk_kern, "out of memory (aborting test)");
        goto finally;
    }

    fct_test__add(fct_xchk_test, chk);
    fctkern__log_chk(fct_xchk_kern, chk);
finally:
    fct_xchk_lineno =0;
    fct_xchk_file =NULL;
    fct_xchk_test =NULL;
    fct_xchk_kern =NULL;
    return is_pass;
}


static int
fct_xchk2_fn(const char *condition, int is_pass, char const *format, ...)
{
    int r =0;
    va_list args;
    va_start(args, format);
    r = _fct_xchk_fn_varg(condition, is_pass, format, args);
    va_end(args);
    return r;
}


static int
fct_xchk_fn(int is_pass, char const *format, ...)
{
    int r=0;
    va_list args;
    va_start(args, format);
    r = _fct_xchk_fn_varg("<none-from-xchk>", is_pass, format, args);
    va_end(args);
    return r;
}


/* Call this with the following argument list:

   fct_xchk(test_condition, format_str, ...)

the bulk of this macro presets some globals to allow us to support
variable argument lists on older compilers. The idea came from the APR
libraries error checking routines. */
#define fct_xchk  fct_xchk_kern = fctkern_ptr__,\
                  fct_xchk_test = fctkern_ptr__->ns.curr_test,\
                  fct_xchk_lineno =__LINE__,\
                  fct_xchk_file=__FILE__,\
                  fct_xchk_fn

#define fct_xchk2  fct_xchk_kern = fctkern_ptr__,\
                   fct_xchk_test = fctkern_ptr__->ns.curr_test,\
                   fct_xchk_lineno =__LINE__,\
                   fct_xchk_file=__FILE__,\
                   fct_xchk2_fn


/* This checks the condition and reports the condition as a string
if it fails. */
#define fct_chk(_CNDTN_)  (fct_xchk((_CNDTN_) ? 1 : 0, #_CNDTN_))

#define _fct_req(_CNDTN_)  \
    if ( !(fct_xchk((_CNDTN_) ? 1 : 0, #_CNDTN_)) ) { break; }


/* When in test mode, construct a mock test object for fct_xchk to operate
with. If we fail a setup up, then we go directly to a teardown mode. */
#define fct_req(_CNDTN_) 				                 \
    if ( fct_ts__is_test_mode(fctkern_ptr__->ns.ts_curr) ) {             \
       _fct_req((_CNDTN_));                                              \
    }                                                                    \
    else if ( fct_ts__is_setup_mode(fctkern_ptr__->ns.ts_curr)           \
              || fct_ts__is_teardown_mode(fctkern_ptr__->ns.ts_curr) ) { \
       fctkern_ptr__->ns.curr_test = fct_ts__make_abort_test(            \
            fctkern_ptr__->ns.ts_curr                                    \
            );                                                           \
       if ( !(fct_xchk((_CNDTN_) ? 1 : 0, #_CNDTN_)) ) {                 \
           fct_ts__setup_abort(fctkern_ptr__->ns.ts_curr);               \
           fct_ts__add_test(                                             \
                fctkern_ptr__->ns.ts_curr, fctkern_ptr__->ns.curr_test   \
                );                                                       \
       }                                                                 \
    } else {                                                             \
       assert("invalid condition for fct_req!");                         \
       _fct_req((_CNDTN_));                                              \
    }


#define fct_chk_eq_dbl(V1, V2) \
    fct_xchk(\
        ((int)(fabs((V1)-(V2)) < DBL_EPSILON)),\
        "chk_eq_dbl: %f != %f",\
        (V1),\
        (V2)\
        )


#define fct_chk_neq_dbl(V1, V2) \
    fct_xchk(\
        ((int)(fabs((V1)-(V2)) >= DBL_EPSILON)),\
        "chk_neq_dbl: %f == %f",\
        (V1),\
        (V2)\
        )


#define fct_chk_eq_str(V1, V2) \
    fct_xchk(fctstr_eq((V1), (V2)),\
          "chk_eq_str: '%s' != '%s'",\
          (V1),\
          (V2)\
          )


#define fct_chk_neq_str(V1, V2) \
    fct_xchk(!fctstr_eq((V1), (V2)),\
          "chk_neq_str: '%s' == '%s'",\
          (V1),\
          (V2)\
          )

/* To quiet warnings with GCC, who think we are being silly and passing
in NULL to strlen, we will filter the predicate through these little
functions */
static int
_fct_chk_empty_str(char const *s)
{
    if ( s == NULL )
    {
        return 1;
    }
    return strlen(s) ==0;
}
static int
_fct_chk_full_str(char const *s)
{
    if ( s == NULL )
    {
        return 0;
    }
    return strlen(s) >0;
}


#define fct_chk_empty_str(V) \
    fct_xchk(_fct_chk_empty_str((V)),\
             "string not empty: '%s'",\
             (V)\
             )

#define fct_chk_full_str(V) \
    fct_xchk(_fct_chk_full_str((V)),\
             "string is full: '%s'",\
             (V)\
             )


#define fct_chk_eq_istr(V1, V2) \
    fct_xchk(fctstr_ieq((V1), (V2)),\
          "chk_eq_str: '%s' != '%s'",\
          (V1),\
          (V2)\
          )


#define fct_chk_neq_istr(V1, V2) \
    fct_xchk(!fctstr_ieq((V1), (V2)),\
          "chk_neq_str: '%s' == '%s'",\
          (V1),\
          (V2)\
          )


#define fct_chk_endswith_str(STR, CHECK)\
    fct_xchk(fctstr_endswith((STR),(CHECK)),\
            "fct_chk_endswith_str: '%s' doesn't end with '%s'",\
            (STR),\
            (CHECK)\
            )


#define fct_chk_iendswith_str(STR, CHECK)\
    fct_xchk(fctstr_iendswith((STR), (CHECK)),\
             "fch_chk_iendswith_str: '%s' doesn't end with '%s'.",\
             (STR),\
             (CHECK)\
             )

#define fct_chk_excl_str(STR, CHECK_EXCLUDE) \
    fct_xchk(!fctstr_incl((STR), (CHECK_EXCLUDE)),\
	  "fct_chk_excl_str: '%s' is included in '%s'",\
	  (STR),\
          (CHECK_EXCLUDE)\
	  )

#define fct_chk_excl_istr(ISTR, ICHECK_EXCLUDE) \
    fct_xchk(!fctstr_iincl((ISTR), (ICHECK_EXCLUDE)),\
	  "fct_chk_excl_istr (case insensitive): '%s' is "\
          "included in'%s'",\
          (ISTR),\
          (ICHECK_EXCLUDE)\
          )

#define fct_chk_incl_str(STR, CHECK_INCLUDE) \
    fct_xchk(fctstr_incl((STR), (CHECK_INCLUDE)),\
          "fct_chk_incl_str: '%s' does not include '%s'",\
	      (STR),\
          (CHECK_INCLUDE)\
	  )


#define fct_chk_incl_istr(ISTR, ICHECK_INCLUDE) \
    fct_xchk(fctstr_iincl((ISTR), (ICHECK_INCLUDE)),\
          "fct_chk_incl_istr (case insensitive): '%s' does "\
          "not include '%s'",\
	      (ISTR),\
          (ICHECK_INCLUDE)\
	  )


#define fct_chk_startswith_str(STR, CHECK)\
    fct_xchk(fctstr_startswith((STR), (CHECK)),\
          "'%s' does not start with '%s'",\
          (STR),\
          (CHECK)\
    )


#define fct_chk_startswith_istr(STR, CHECK)\
    fct_xchk(fctstr_istartswith((STR), (CHECK)),\
          "case insensitive check: '%s' does not start with '%s'",\
          (STR),\
          (CHECK)\
    )

#define fct_chk_eq_int(V1, V2) \
    fct_xchk(\
        ((V1) == (V2)),\
        "chq_eq_int: %d != %d",\
        (V1),\
        (V2)\
        )


#define fct_chk_neq_int(V1, V2) \
    fct_xchk(\
        ((V1) != (V2)),\
        "chq_neq_int: %d == %d",\
        (V1),\
        (V2)\
        )

#define fct_chk_ex(EXCEPTION, CODE)   \
   {                                  \
      bool pass_chk_ex = false;       \
      try {                           \
          CODE;                       \
          pass_chk_ex = false;        \
      } catch ( EXCEPTION ) {         \
          pass_chk_ex = true;         \
      } catch ( ... ) {               \
          pass_chk_ex = false;        \
      }                               \
      fct_xchk(                       \
	pass_chk_ex,                  \
        "%s exception not generated", \
        #EXCEPTION                    \
      );                              \
   }                                  \

/*
---------------------------------------------------------
GUT CHECK MACROS
----------------------------------------------------------

The following macros are used to help check the "guts" of
the FCT, and to confirm that it all works according to spec.
*/

/* Generates a message to STDERR and exits the application with a
non-zero number. */
#define _FCT_GUTCHK(_CNDTN_) \
   if ( !(_CNDTN_) ) {\
      fprintf(stderr, "gutchk fail: '"  #_CNDTN_ "' was not true.\n");\
      exit(1);\
   }\
   else {\
      fprintf(stdout, "gutchk pass:  '" #_CNDTN_ "'\n");\
   }

/*
---------------------------------------------------------
MULTI-FILE TEST SUITE MACROS
----------------------------------------------------------

I struggled trying to figure this out in a way that was
as simple as possible. I wanted to be able to define
the test suite in one object file, then refer it within
the other one within the minimum amount of typing.

Unfortunately without resorting to some supermacro
work, I could only find a happy comprimise.

See test_multi.c for an example.
*/

/* The following macros are used in your separate object
file to define your test suite.  */


#define FCTMF_FIXTURE_SUITE_BGN(NAME) \
	void NAME (fctkern_t *fctkern_ptr__) {\
        FCT_REFERENCE_FUNCS();\
        FCT_FIXTURE_SUITE_BGN( NAME ) {

#define FCTMF_FIXTURE_SUITE_END() \
		} FCT_FIXTURE_SUITE_END();\
	}

#define FCTMF_SUITE_BGN(NAME) \
	void NAME (fctkern_t *fctkern_ptr__) {\
        FCT_REFERENCE_FUNCS();\
        FCT_SUITE_BGN( NAME ) {
#define FCTMF_SUITE_END() \
       } FCT_SUITE_END(); \
   }


/* Deprecated, no longer required. */
#define FCTMF_SUITE_DEF(NAME)


/* Executes a test suite defined by FCTMF_SUITE* */
#define FCTMF_SUITE_CALL(NAME)  {\
    void NAME (fctkern_t *);\
    NAME (fctkern_ptr__);\
    }


/*
---------------------------------------------------------
FCT QUICK TEST API
----------------------------------------------------------
The goal of these little macros is to try and get you
up and running with a test as quick as possible.

The basic idea is that there is one test per test suite.
*/

#define FCT_QTEST_BGN(NAME) \
	FCT_SUITE_BGN(NAME) {\
		FCT_TEST_BGN(NAME) {\

#define FCT_QTEST_END() \
		} FCT_TEST_END();\
	} FCT_SUITE_END();


#define FCT_QTEST_BGN_IF(_CONDITION_, _NAME_) \
	FCT_SUITE_BGN(_NAME_) {\
		FCT_TEST_BGN_IF(_CONDITION_, _NAME_) {\

#define FCT_QTEST_END_IF() \
		} FCT_TEST_END_IF();\
	} FCT_SUITE_END();

#endif /* !FCT_INCLUDED__IMB */
