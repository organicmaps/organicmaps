/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_SUCC_PRED_HPP_JOFA_080913
#define BOOST_ICL_TYPE_TRAITS_SUCC_PRED_HPP_JOFA_080913

namespace boost{ namespace icl
{
    template <class IncrementableT>
    inline static IncrementableT succ(IncrementableT x) { return ++x; }

    template <class DecrementableT>
    inline static DecrementableT pred(DecrementableT x) { return --x; }

}} // namespace boost icl

#endif


