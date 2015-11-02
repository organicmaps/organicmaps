/*=============================================================================
    Copyright (c) 2015 Kohei Takahashi

    Use modification and distribution are subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/
#ifndef FUSION_DETAIL_ENABLER_02082015_163810
#define FUSION_DETAIL_ENABLER_02082015_163810

namespace boost { namespace fusion { namespace detail
{
    template <typename, typename T = void>
    struct enabler { typedef T type; };
}}}

#endif
