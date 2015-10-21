//  (C) Copyright Gennadiy Rozental 2005-2014.
//  Use, modification, and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : defines variable modifiers
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_ENV_MODIFIER_HPP
#define BOOST_TEST_UTILS_RUNTIME_ENV_MODIFIER_HPP

// Boost.Runtime.Parameter
#include <boost/test/utils/runtime/config.hpp>

// Boost.Test
#include <boost/test/utils/named_params.hpp>

namespace boost {

namespace BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE {

namespace environment {

// ************************************************************************** //
// **************         environment variable modifiers       ************** //
// ************************************************************************** //

namespace {

nfp::typed_keyword<cstring,struct global_id_t>   global_id;
nfp::keyword<struct default_value_t>             default_value;
nfp::keyword<struct interpreter_t>               interpreter;

} // local namespace
} // namespace environment

} // namespace BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE

} // namespace boost

#endif // BOOST_TEST_UTILS_RUNTIME_ENV_MODIFIER_HPP
