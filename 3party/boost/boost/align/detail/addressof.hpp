/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_DETAIL_ADDRESSOF_HPP
#define BOOST_ALIGN_DETAIL_ADDRESSOF_HPP

#include <boost/config.hpp>

#if !defined(BOOST_NO_CXX11_ADDRESSOF)
#include <memory>
#else
#include <boost/core/addressof.hpp>
#endif

namespace boost {
    namespace alignment {
        namespace detail {
#if !defined(BOOST_NO_CXX11_ADDRESSOF)
            using std::addressof;
#else
            using boost::addressof;
#endif
        }
    }
}

#endif
