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

#ifndef BOOST_TEST_UTILS_RUNTIME_CONFIGURATION_HPP
#define BOOST_TEST_UTILS_RUNTIME_CONFIGURATION_HPP

// Boost.Runtime.Parameter
#include <boost/test/utils/runtime/config.hpp>
#include <boost/test/utils/runtime/parameter.hpp>
#include <boost/test/utils/runtime/argument.hpp>

namespace boost {

namespace BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE {

// ************************************************************************** //
// **************            runtime::configuration            ************** //
// ************************************************************************** //

class config_source {
    virtual parameter const& config_param_begin() const = 0;
    virtual parameter const& config_param_end() const   = 0;

protected:
    config_source()     {}
    ~config_source()    {}
};

// ************************************************************************** //
// **************            runtime::configuration            ************** //
// ************************************************************************** //

template<typename StoragePolicy,typename IdentificationPlicy,typename ConflictResolutionPolicy>
class configuration : public StoragePolicy, public IdentificationPlicy, public ConflictResolutionPolicy {
public:
    // Constructor
    configuration();

    void    use( config_source const& )
    {

    }
private:
};

} // namespace BOOST_TEST_UTILS_RUNTIME_PARAM_NAMESPACE

} // namespace boost

#endif // BOOST_TEST_UTILS_RUNTIME_CONFIGURATION_HPP
