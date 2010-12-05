/*
    Copyright 2005-2007 Adobe Systems Incorporated
   
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
*/

/*************************************************************************************************/

#ifndef BOOST_UNORDERED_DETAIL_MOVE_HEADER
#define BOOST_UNORDERED_DETAIL_MOVE_HEADER

#include <boost/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/not.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/detail/workaround.hpp>

/*************************************************************************************************/

#if defined(BOOST_NO_SFINAE)
#  define BOOST_UNORDERED_NO_HAS_MOVE_ASSIGN
#elif defined(__GNUC__) && \
    (__GNUC__ < 3 || __GNUC__ == 3 && __GNUC_MINOR__ <= 3)
#  define BOOST_UNORDERED_NO_HAS_MOVE_ASSIGN
#elif BOOST_WORKAROUND(BOOST_INTEL, < 900) || \
    BOOST_WORKAROUND(__EDG_VERSION__, < 304) || \
    BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x0593))
#  define BOOST_UNORDERED_NO_HAS_MOVE_ASSIGN
#endif

/*************************************************************************************************/

namespace boost {
namespace unordered_detail {

/*************************************************************************************************/

namespace move_detail {

/*************************************************************************************************/

#if !defined(BOOST_UNORDERED_NO_HAS_MOVE_ASSIGN)

/*************************************************************************************************/

template <typename T>  
struct class_has_move_assign {  
    class type {
        typedef T& (T::*E)(T t);  
        typedef char (&no_type)[1];  
        typedef char (&yes_type)[2];  
        template <E e> struct sfinae { typedef yes_type type; };  
        template <class U>  
        static typename sfinae<&U::operator=>::type test(int);  
        template <class U>  
        static no_type test(...);  
    public:  
        enum {value = sizeof(test<T>(1)) == sizeof(yes_type)};  
    };
 };  

/*************************************************************************************************/

template<typename T>
struct has_move_assign : boost::mpl::and_<boost::is_class<T>, class_has_move_assign<T> > {};

/*************************************************************************************************/

class test_can_convert_anything { };

/*************************************************************************************************/

#endif // BOOST_UNORDERED_NO_HAS_MOVE_ASSIGN

/*************************************************************************************************/

/*
    REVISIT (sparent@adobe.com): This is a work around for Boost 1.34.1 and VC++ 2008 where
    boost::is_convertible<T, T> fails to compile.
*/

template <typename T, typename U>
struct is_convertible : boost::mpl::or_<
    boost::is_same<T, U>,
    boost::is_convertible<T, U>
> { };

/*************************************************************************************************/

} //namespace move_detail


/*************************************************************************************************/

/*!
\ingroup move_related
\brief move_from is used for move_ctors.
*/

template <typename T>
struct move_from
{
    explicit move_from(T& x) : source(x) { }
    T& source;
private:
    move_from& operator=(move_from const&);
};

/*************************************************************************************************/

#if !defined(BOOST_UNORDERED_NO_HAS_MOVE_ASSIGN)

/*************************************************************************************************/

/*!
\ingroup move_related
\brief The is_movable trait can be used to identify movable types.
*/
template <typename T>
struct is_movable : boost::mpl::and_<
                        boost::is_convertible<move_from<T>, T>,
                        move_detail::has_move_assign<T>,
                        boost::mpl::not_<boost::is_convertible<move_detail::test_can_convert_anything, T> >
                    > { };

/*************************************************************************************************/

#else // BOOST_UNORDERED_NO_HAS_MOVE_ASSIGN

// On compilers which don't have adequate SFINAE support, treat most types as unmovable,
// unless the trait is specialized.

template <typename T>
struct is_movable : boost::mpl::false_ { };

#endif

/*************************************************************************************************/

#if !defined(BOOST_NO_SFINAE)

/*************************************************************************************************/

/*!
\ingroup move_related
\brief copy_sink and move_sink are used to select between overloaded operations according to
 whether type T is movable and convertible to type U.
\sa move
*/

template <typename T,
          typename U = T,
          typename R = void*>
struct copy_sink : boost::enable_if<
                        boost::mpl::and_<
                            boost::unordered_detail::move_detail::is_convertible<T, U>,                           
                            boost::mpl::not_<is_movable<T> >
                        >,
                        R
                    >
{ };

/*************************************************************************************************/

/*!
\ingroup move_related
\brief move_sink and copy_sink are used to select between overloaded operations according to
 whether type T is movable and convertible to type U.
 \sa move
*/

template <typename T,
          typename U = T,
          typename R = void*>
struct move_sink : boost::enable_if<
                        boost::mpl::and_<
                            boost::unordered_detail::move_detail::is_convertible<T, U>,                            
                            is_movable<T>
                        >,
                        R
                    >
{ };

/*************************************************************************************************/

/*!
\ingroup move_related
\brief This version of move is selected when T is_movable . It in turn calls the move
constructor. This call, with the help of the return value optimization, will cause x to be moved
instead of copied to its destination. See adobe/test/move/main.cpp for examples.

*/
template <typename T>
T move(T& x, typename move_sink<T>::type = 0) { return T(move_from<T>(x)); }

/*************************************************************************************************/

/*!
\ingroup move_related
\brief This version of move is selected when T is not movable . The net result will be that
x gets copied.
*/
template <typename T>
T& move(T& x, typename copy_sink<T>::type = 0) { return x; }

/*************************************************************************************************/

#else // BOOST_NO_SFINAE

// On compilers without SFINAE, define copy_sink to always use the copy function.

template <typename T,
          typename U = T,
          typename R = void*>
struct copy_sink
{
    typedef R type;
};

// Always copy the element unless this is overloaded.

template <typename T>
T& move(T& x) {
    return x;
}

#endif // BOOST_NO_SFINAE

} // namespace unordered_detail
} // namespace boost

/*************************************************************************************************/

#endif

/*************************************************************************************************/
