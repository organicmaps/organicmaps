/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_POINTER_TO_OTHER_HPP
#define BOOST_INTRUSIVE_POINTER_TO_OTHER_HPP

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/version.hpp>

#if (BOOST_VERSION < 103400)

#ifndef BOOST_POINTER_TO_OTHER_HPP_INCLUDED
#define BOOST_POINTER_TO_OTHER_HPP_INCLUDED

namespace boost {

template<class T, class U>
   struct pointer_to_other;

template<class T, class U, template <class> class Sp>
   struct pointer_to_other< Sp<T>, U >
{
   typedef Sp<U> type;
};

template<class T, class T2, class U,
        template <class, class> class Sp>
   struct pointer_to_other< Sp<T, T2>, U >
{
   typedef Sp<U, T2> type;
};

template<class T, class T2, class T3, class U,
        template <class, class, class> class Sp>
struct pointer_to_other< Sp<T, T2, T3>, U >
{
   typedef Sp<U, T2, T3> type;
};

template<class T, class U>
struct pointer_to_other< T*, U > 
{
   typedef U* type;
};

} // namespace boost

#endif

#else

#include <boost/pointer_to_other.hpp>

#endif   //#ifndef BOOST_POINTER_TO_OTHER_HPP_INCLUDED

#include <boost/intrusive/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTRUSIVE_POINTER_TO_OTHER_HPP
