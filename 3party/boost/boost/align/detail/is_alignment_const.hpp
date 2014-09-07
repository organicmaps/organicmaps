/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_DETAIL_IS_ALIGNMENT_CONST_HPP
#define BOOST_ALIGN_DETAIL_IS_ALIGNMENT_CONST_HPP

#include <cstddef>

namespace boost {
    namespace alignment {
        namespace detail {
            template<std::size_t N>
            struct is_alignment_const {
                enum {
                    value = (N > 0) && ((N & (N - 1)) == 0)
                };
            };
        }
    }
}

#endif
