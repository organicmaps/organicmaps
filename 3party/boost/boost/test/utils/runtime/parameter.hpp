//  (C) Copyright Gennadiy Rozental 2005-2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : abstract interface for the formal parameter
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_PARAMETER_HPP
#define BOOST_TEST_UTILS_RUNTIME_PARAMETER_HPP

// Boost.Runtime.Parameter
#include <boost/test/utils/runtime/config.hpp>

namespace boost {

namespace BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE {

// ************************************************************************** //
// **************              runtime::parameter              ************** //
// ************************************************************************** //

class parameter {
public:
    virtual ~parameter() {}
};

} // namespace BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE

} // namespace boost

#endif // BOOST_TEST_UTILS_RUNTIME_PARAMETER_HPP
