// Copyright Alexander Nasonov 2006-2009
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef FILE_boost_scope_exit_hpp_INCLUDED
#define FILE_boost_scope_exit_hpp_INCLUDED

#include <boost/config.hpp>

#include <boost/detail/workaround.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/cat.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/typeof/typeof.hpp>

#if defined(__GNUC__) && !defined(BOOST_INTEL)
# define BOOST_SCOPE_EXIT_AUX_GCC (__GNUC__ * 100 + __GNUC_MINOR__)
#else
# define BOOST_SCOPE_EXIT_AUX_GCC 0
#endif

#if BOOST_WORKAROUND(BOOST_SCOPE_EXIT_AUX_GCC, BOOST_TESTED_AT(413))
#define BOOST_SCOPE_EXIT_AUX_TPL_WORKAROUND
#endif

// Steven Watanabe's trick with a modification suggested by Kim Barrett
namespace boost { namespace scope_exit { namespace aux {

    // Type of a local boost_scope_exit_args variable.
    // First use in a local scope will declare the boost_scope_exit_args
    // variable, subsequent uses will be resolved as two comparisons
    // (cmp1 with 0 and cmp2 with boost_scope_exit_args).
    template<int Dummy = 0>
    struct declared
    {
        void* value;
        static int const cmp2 = 0;
        friend void operator>(int, declared const&) {}
    };

    struct undeclared { declared<> dummy[2]; };

    template<int> struct resolve;

    template<>
    struct resolve<sizeof(declared<>)>
    {
        static const int cmp1 = 0;
    };

    template<>
    struct resolve<sizeof(undeclared)>
    {
        template<int>
        struct cmp1
        {
            static int const cmp2 = 0;
        };
    };
} } }

extern boost::scope_exit::aux::undeclared boost_scope_exit_args; // undefined


namespace boost { namespace scope_exit { namespace aux {

typedef void (*ref_tag)(int&);
typedef void (*val_tag)(int );

template<class T, class Tag> struct member;

template<class T>
struct member<T,ref_tag>
{
    T& value;
#ifndef BOOST_SCOPE_EXIT_AUX_TPL_WORKAROUND
    member(T& ref) : value(ref) {}
#endif
};

template<class T>
struct member<T,val_tag>
{
    T value;
#ifndef BOOST_SCOPE_EXIT_AUX_TPL_WORKAROUND
    member(T& val) : value(val) {}
#endif
};

template<class T> inline T& deref(T* p, ref_tag) { return *p; }
template<class T> inline T& deref(T& r, val_tag) { return  r; }

template<class T>
struct wrapper
{
    typedef T type;
};

template<class T> wrapper<T> wrap(T&);

} } }

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::scope_exit::aux::wrapper, 1)

#define BOOST_SCOPE_EXIT_AUX_GUARD(id)     BOOST_PP_CAT(boost_se_guard_,    id)
#define BOOST_SCOPE_EXIT_AUX_GUARD_T(id)   BOOST_PP_CAT(boost_se_guard_t_,  id)
#define BOOST_SCOPE_EXIT_AUX_PARAMS(id)    BOOST_PP_CAT(boost_se_params_,   id)
#define BOOST_SCOPE_EXIT_AUX_PARAMS_T(id)  BOOST_PP_CAT(boost_se_params_t_, id)

#define BOOST_SCOPE_EXIT_AUX_TAG(id, i) \
    BOOST_PP_SEQ_CAT( (boost_se_tag_)(i)(_)(id) )

#define BOOST_SCOPE_EXIT_AUX_PARAM(id, i, var) \
    BOOST_PP_SEQ_CAT( (boost_se_param_)(i)(_)(id) )

#define BOOST_SCOPE_EXIT_AUX_PARAM_T(id, i, var) \
    BOOST_PP_SEQ_CAT( (boost_se_param_t_)(i)(_)(id) )

#define BOOST_SCOPE_EXIT_AUX_CAPTURE_T(id, i, var) \
    BOOST_PP_SEQ_CAT( (boost_se_capture_t_)(i)(_)(id) )

#define BOOST_SCOPE_EXIT_AUX_WRAPPED(id, i) \
    BOOST_PP_SEQ_CAT( (boost_se_wrapped_t_)(i)(_)(id) )

#define BOOST_SCOPE_EXIT_AUX_DEREF(id, i, var) \
    boost::scope_exit::aux::deref(var, (BOOST_SCOPE_EXIT_AUX_TAG(id,i))0)

#define BOOST_SCOPE_EXIT_AUX_MEMBER(r, id, i, var) \
    boost::scope_exit::aux::member<                \
        BOOST_SCOPE_EXIT_AUX_PARAM_T(id,i,var),    \
        BOOST_SCOPE_EXIT_AUX_TAG(id,i)             \
    > BOOST_SCOPE_EXIT_AUX_PARAM(id,i,var);

// idty is (id,typename) or (id,BOOST_PP_EMPTY())
#define BOOST_SCOPE_EXIT_AUX_ARG_DECL(r, idty, i, var)             \
    BOOST_PP_COMMA_IF(i) BOOST_PP_TUPLE_ELEM(2,1,idty)             \
    BOOST_SCOPE_EXIT_AUX_PARAMS_T(BOOST_PP_TUPLE_ELEM(2,0,idty)):: \
    BOOST_SCOPE_EXIT_AUX_PARAM_T(BOOST_PP_TUPLE_ELEM(2,0,idty), i, var) var
 
#define BOOST_SCOPE_EXIT_AUX_ARG(r, id, i, var) BOOST_PP_COMMA_IF(i) \
    boost_se_params_->BOOST_SCOPE_EXIT_AUX_PARAM(id,i,var).value

#define BOOST_SCOPE_EXIT_AUX_TAG_DECL(r, id, i, var) \
    typedef void (*BOOST_SCOPE_EXIT_AUX_TAG(id,i))(int var);


#ifdef BOOST_SCOPE_EXIT_AUX_TPL_WORKAROUND

#define BOOST_SCOPE_EXIT_AUX_PARAMS_T_CTOR(id, seq)

#define BOOST_SCOPE_EXIT_AUX_PARAM_INIT(r, id, i, var) \
    BOOST_PP_COMMA_IF(i) { BOOST_SCOPE_EXIT_AUX_DEREF(id,i,var) }

#define BOOST_SCOPE_EXIT_AUX_PARAMS_INIT(id, seq) \
    = { BOOST_PP_SEQ_FOR_EACH_I(BOOST_SCOPE_EXIT_AUX_PARAM_INIT, id, seq) };

#else

#define BOOST_SCOPE_EXIT_AUX_CTOR_ARG(r, id, i, var) BOOST_PP_COMMA_IF(i) \
    BOOST_SCOPE_EXIT_AUX_PARAM_T(id,i,var) & BOOST_PP_CAT(a,i)

#define BOOST_SCOPE_EXIT_AUX_MEMBER_INIT(r, id, i, var) BOOST_PP_COMMA_IF(i) \
    BOOST_SCOPE_EXIT_AUX_PARAM(id,i,var) ( BOOST_PP_CAT(a,i) )

#define BOOST_SCOPE_EXIT_AUX_PARAMS_T_CTOR(id, seq)                        \
    BOOST_SCOPE_EXIT_AUX_PARAMS_T(id)(                                     \
        BOOST_PP_SEQ_FOR_EACH_I(BOOST_SCOPE_EXIT_AUX_CTOR_ARG, id, seq ) ) \
    : BOOST_PP_SEQ_FOR_EACH_I(BOOST_SCOPE_EXIT_AUX_MEMBER_INIT, id, seq) {}

#define BOOST_SCOPE_EXIT_AUX_PARAM_INIT(r, id, i, var) \
    BOOST_PP_COMMA_IF(i) BOOST_SCOPE_EXIT_AUX_DEREF(id,i,var)

#define BOOST_SCOPE_EXIT_AUX_PARAMS_INIT(id, seq) \
    ( BOOST_PP_SEQ_FOR_EACH_I(BOOST_SCOPE_EXIT_AUX_PARAM_INIT, id, seq) );

#endif

#if defined(BOOST_TYPEOF_EMULATION)

#define BOOST_SCOPE_EXIT_AUX_CAPTURE_DECL(r, idty, i, var)                   \
    struct BOOST_SCOPE_EXIT_AUX_WRAPPED(BOOST_PP_TUPLE_ELEM(2,0,idty), i)    \
        : BOOST_TYPEOF(boost::scope_exit::aux::wrap(                         \
        BOOST_SCOPE_EXIT_AUX_DEREF(BOOST_PP_TUPLE_ELEM(2,0,idty), i, var)))  \
    {}; typedef BOOST_PP_TUPLE_ELEM(2,1,idty)                                \
        BOOST_SCOPE_EXIT_AUX_WRAPPED(BOOST_PP_TUPLE_ELEM(2,0,idty), i)::type \
        BOOST_SCOPE_EXIT_AUX_CAPTURE_T(BOOST_PP_TUPLE_ELEM(2,0,idty), i, var);

#elif defined(BOOST_INTEL)

#define BOOST_SCOPE_EXIT_AUX_CAPTURE_DECL(r, idty, i, var)                 \
    typedef BOOST_TYPEOF_KEYWORD(                                          \
        BOOST_SCOPE_EXIT_AUX_DEREF(BOOST_PP_TUPLE_ELEM(2,0,idty), i, var)) \
        BOOST_SCOPE_EXIT_AUX_CAPTURE_T(BOOST_PP_TUPLE_ELEM(2,0,idty), i, var);

#else

#define BOOST_SCOPE_EXIT_AUX_CAPTURE_DECL(r, idty, i, var)                   \
    typedef BOOST_TYPEOF(boost::scope_exit::aux::wrap(                       \
        BOOST_SCOPE_EXIT_AUX_DEREF(BOOST_PP_TUPLE_ELEM(2,0,idty), i, var)))  \
        BOOST_SCOPE_EXIT_AUX_WRAPPED(BOOST_PP_TUPLE_ELEM(2,0,idty), i);      \
    typedef BOOST_PP_TUPLE_ELEM(2,1,idty)                                    \
        BOOST_SCOPE_EXIT_AUX_WRAPPED(BOOST_PP_TUPLE_ELEM(2,0,idty), i)::type \
        BOOST_SCOPE_EXIT_AUX_CAPTURE_T(BOOST_PP_TUPLE_ELEM(2,0,idty), i, var);

#endif

#define BOOST_SCOPE_EXIT_AUX_PARAM_DECL(r, idty, i, var) \
    typedef BOOST_SCOPE_EXIT_AUX_CAPTURE_T(              \
        BOOST_PP_TUPLE_ELEM(2,0,idty), i, var)           \
        BOOST_SCOPE_EXIT_AUX_PARAM_T(BOOST_PP_TUPLE_ELEM(2,0,idty), i, var);


#define BOOST_SCOPE_EXIT_AUX_IMPL(id, seq, ty)                                 \
    BOOST_PP_SEQ_FOR_EACH_I(BOOST_SCOPE_EXIT_AUX_TAG_DECL, id, seq)            \
    BOOST_PP_SEQ_FOR_EACH_I(BOOST_SCOPE_EXIT_AUX_CAPTURE_DECL, (id,ty), seq)   \
    struct BOOST_SCOPE_EXIT_AUX_PARAMS_T(id) {                                 \
        BOOST_PP_SEQ_FOR_EACH_I(BOOST_SCOPE_EXIT_AUX_PARAM_DECL, (id,ty), seq) \
        BOOST_PP_SEQ_FOR_EACH_I(BOOST_SCOPE_EXIT_AUX_MEMBER, id, seq)          \
        BOOST_SCOPE_EXIT_AUX_PARAMS_T_CTOR(id, seq)                            \
    } BOOST_SCOPE_EXIT_AUX_PARAMS(id) BOOST_SCOPE_EXIT_AUX_PARAMS_INIT(id,seq) \
    boost::scope_exit::aux::declared< boost::scope_exit::aux::resolve<         \
        sizeof(boost_scope_exit_args)>::cmp1<0>::cmp2 > boost_scope_exit_args; \
    boost_scope_exit_args.value = &BOOST_SCOPE_EXIT_AUX_PARAMS(id);            \
    struct BOOST_SCOPE_EXIT_AUX_GUARD_T(id) {                                  \
        BOOST_SCOPE_EXIT_AUX_PARAMS_T(id)* boost_se_params_;                   \
        BOOST_SCOPE_EXIT_AUX_GUARD_T(id) (void* boost_se_params)               \
            : boost_se_params_(                                                \
                  (BOOST_SCOPE_EXIT_AUX_PARAMS_T(id)*)boost_se_params)         \
        {}                                                                     \
        ~BOOST_SCOPE_EXIT_AUX_GUARD_T(id)() { boost_se_body(                   \
            BOOST_PP_SEQ_FOR_EACH_I(BOOST_SCOPE_EXIT_AUX_ARG, id, seq) ); }    \
        static void boost_se_body(BOOST_PP_SEQ_FOR_EACH_I(                     \
            BOOST_SCOPE_EXIT_AUX_ARG_DECL, (id,ty), seq) )

#if defined(BOOST_MSVC)

#define BOOST_SCOPE_EXIT_END } BOOST_SCOPE_EXIT_AUX_GUARD(__COUNTER__) ( \
    boost_scope_exit_args.value);

#define BOOST_SCOPE_EXIT(seq) \
    BOOST_SCOPE_EXIT_AUX_IMPL(__COUNTER__, seq, BOOST_PP_EMPTY())

#else

#define BOOST_SCOPE_EXIT_END } BOOST_SCOPE_EXIT_AUX_GUARD(__LINE__) ( \
    boost_scope_exit_args.value);

#define BOOST_SCOPE_EXIT(seq) \
    BOOST_SCOPE_EXIT_AUX_IMPL(__LINE__, seq, BOOST_PP_EMPTY())

#endif

#ifdef BOOST_SCOPE_EXIT_AUX_TPL_WORKAROUND
#define BOOST_SCOPE_EXIT_TPL(seq) \
    BOOST_SCOPE_EXIT_AUX_IMPL(__LINE__, seq, typename)
#else
#define BOOST_SCOPE_EXIT_TPL(seq) BOOST_SCOPE_EXIT(seq)
#endif

#endif // #ifndef FILE_boost_scope_exit_hpp_INCLUDED

