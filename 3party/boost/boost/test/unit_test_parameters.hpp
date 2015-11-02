//  (C) Copyright Gennadiy Rozental 2001-2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// @brief Provides access to various Unit Test Framework runtime parameters
///
/// Primarily for use by the framework itself
// ***************************************************************************

#ifndef BOOST_TEST_UNIT_TEST_PARAMETERS_HPP_071894GER
#define BOOST_TEST_UNIT_TEST_PARAMETERS_HPP_071894GER

#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/detail/log_level.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

// STL
#include <iosfwd>
#include <list>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace runtime_config {

// ************************************************************************** //
// **************                 runtime_config               ************** //
// ************************************************************************** //

BOOST_TEST_DECL void                    init( int& argc, char** argv );

/// Automatically attach debugger in a location of fatal error
BOOST_TEST_DECL bool                    auto_start_dbg();
BOOST_TEST_DECL const_string            break_exec_path();
/// Should we catch system errors/sygnals?
BOOST_TEST_DECL bool                    catch_sys_errors();
/// Should we try to produce color output?
BOOST_TEST_DECL bool                    color_output();
/// Should we detect floating point exceptions?
BOOST_TEST_DECL bool                    detect_fp_exceptions();
/// Should we detect memory leaks (>0)? And if yes, which specific memory allocation should we break.
BOOST_TEST_DECL long                    detect_memory_leaks();
/// List content of test tree?
BOOST_TEST_DECL output_format           list_content();
/// List available labels?
BOOST_TEST_DECL bool                    list_labels();
/// Which output format to use
BOOST_TEST_DECL output_format           log_format();
/// Which log level to set
BOOST_TEST_DECL unit_test::log_level    log_level();
/// Where to direct log stream into
BOOST_TEST_DECL std::ostream*           log_sink();
/// If memory leak detection, where to direct the report
BOOST_TEST_DECL const_string            memory_leaks_report_file();
/// Do not prodce result code
BOOST_TEST_DECL bool                    no_result_code();
/// Random seed to use to randomize order of test units being run
BOOST_TEST_DECL unsigned                random_seed();
/// Which format to use to report results
BOOST_TEST_DECL output_format           report_format();
/// Wht lever of report format to set
BOOST_TEST_DECL unit_test::report_level report_level();
/// Where to direct results report into
BOOST_TEST_DECL std::ostream*           report_sink();
/// Should we save pattern (true) or match against existing pattern (used by output validation tool)
BOOST_TEST_DECL bool                    save_pattern();
/// Should Unit Test framework show the build information?
BOOST_TEST_DECL bool                    show_build_info();
/// Tells Unit Test Framework to show test progress (forces specific log level)
BOOST_TEST_DECL bool                    show_progress();
/// Specific test units to run/exclude
BOOST_TEST_DECL std::list<std::string> const& test_to_run();
/// Should execution monitor use alternative stack for signal handling
BOOST_TEST_DECL bool                    use_alt_stack();
/// Tells Unit Test Framework to wait for debugger to attach
BOOST_TEST_DECL bool                    wait_for_debugger();

} // namespace runtime_config
} // namespace unit_test
} // namespace boost

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UNIT_TEST_PARAMETERS_HPP_071894GER
