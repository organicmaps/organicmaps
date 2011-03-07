/*-----------------------------------------------------------------------------+
Copyright (c) 2007-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_SET_HPP_JOFA_070519
#define BOOST_ICL_SET_HPP_JOFA_070519

#include <boost/icl/impl_config.hpp>

#if defined(ICL_USE_BOOST_INTERPROCESS_IMPLEMENTATION)
#include <boost/interprocess/containers/set.hpp>
#elif defined(ICL_USE_BOOST_MOVE_IMPLEMENTATION)
#include <boost/container/set.hpp>
#else 
#include <set>
#endif

#include <boost/icl/concept/associative_element_container.hpp>


}} // namespace icl boost

#endif // BOOST_ICL_SET_HPP_JOFA_070519

