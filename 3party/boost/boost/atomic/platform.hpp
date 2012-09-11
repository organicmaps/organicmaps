//  Copyright (c) 2009 Helge Bahmann
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>

#if defined(__GNUC__) && (defined(__i386__) || defined(__amd64__))

	#include <boost/atomic/detail/gcc-x86.hpp>

#elif defined(__GNUC__) && defined(__alpha__)

	#include <boost/atomic/detail/gcc-alpha.hpp>

#elif defined(__GNUC__) && (defined(__POWERPC__) || defined(__PPC__))

	#include <boost/atomic/detail/gcc-ppc.hpp>

// This list of ARM architecture versions comes from Apple's arm/arch.h header.
// I don't know how complete it is.
#elif defined(__GNUC__) && (defined(__ARM_ARCH_6__)  || defined(__ARM_ARCH_6J__) \
                         || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__) \
                         || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_7A__))

	#include <boost/atomic/detail/gcc-armv6+.hpp>

#elif defined(__linux__) && defined(__arm__)

	#include <boost/atomic/detail/linux-arm.hpp>

#elif defined(BOOST_USE_WINDOWS_H) || defined(_WIN32_CE) || defined(BOOST_MSVC) || defined(BOOST_INTEL_WIN) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)

	#include <boost/atomic/detail/interlocked.hpp>

#else

	#warning "Using slow fallback atomic implementation"
	#include <boost/atomic/detail/generic-cas.hpp>

#endif
