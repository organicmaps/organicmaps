/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_DETAIL_IS_ALIGNED_HPP
#define BOOST_ALIGN_DETAIL_IS_ALIGNED_HPP

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/align/detail/address.hpp>
#include <boost/align/detail/is_alignment.hpp>
#include <cstddef>

namespace boost {
    namespace alignment {
        inline bool is_aligned(std::size_t alignment,
            const void* ptr) BOOST_NOEXCEPT
        {
            BOOST_ASSERT(detail::is_alignment(alignment));
            return (detail::address_t(ptr) & (alignment - 1)) == 0;
        }
    }
}

#endif
