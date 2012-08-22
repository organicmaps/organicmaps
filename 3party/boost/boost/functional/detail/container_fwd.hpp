
// Copyright 2005-2012 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Forwarding header for container_fwd.hpp's new location.
// This header is deprecated, I'll change the warning to an error in a future
// release, and then later remove the header completely.

#if !defined(BOOST_FUNCTIONAL_DETAIL_CONTAINER_FWD_HPP)
#define BOOST_FUNCTIONAL_DETAIL_CONTAINER_FWD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#if defined(__EDG__)
#elif defined(_MSC_VER) || defined(__BORLANDC__) || defined(__DMC__)
#pragma message("Warning: boost/functional/detail/container_fwd.hpp is deprecated, use boost/detail/container_fwd.hpp instead.")
#elif defined(__GNUC__) || defined(__HP_aCC) || \
    defined(__SUNPRO_CC) || defined(__IBMCPP__)
#warning "boost/functional/detail/container_fwd.hpp is deprecated, use boost/detail/container_fwd.hpp instead."
#endif

#include <boost/detail/container_fwd.hpp>

#endif
