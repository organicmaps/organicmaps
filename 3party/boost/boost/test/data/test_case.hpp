//  (C) Copyright Gennadiy Rozental 2011-2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief test case family based on data generator
// ***************************************************************************

#ifndef BOOST_TEST_DATA_TEST_CASE_HPP_102211GER
#define BOOST_TEST_DATA_TEST_CASE_HPP_102211GER

// Boost.Test
#include <boost/test/data/config.hpp>
#include <boost/test/data/dataset.hpp>

// Boost
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/comparison/equal.hpp>

#include <boost/bind.hpp>

#include <boost/test/detail/suppress_warnings.hpp>
#include <boost/test/tools/detail/print_helper.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace data {

// ************************************************************************** //
// **************              test_case_template              ************** //
// ************************************************************************** //

namespace ds_detail {

template<typename TestCase,typename DataSet>
class test_case_gen : public test_unit_generator {
public:
    // Constructor
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    test_case_gen( const_string tc_name, const_string tc_file, std::size_t tc_line, DataSet&& ds )
    : m_tc_name( ut_detail::normalize_test_case_name( tc_name ) )
    {
        data::for_each_sample( std::forward<DataSet>( ds ), *this );
    }
    test_case_gen( test_case_gen&& gen )
    : m_tc_name( gen.m_tc_name )
    , m_test_cases( std::move(gen.m_test_cases) )
    {}
#else
    test_case_gen( const_string tc_name, const_string tc_file, std::size_t tc_line, DataSet const& ds )
    : m_tc_name( ut_detail::normalize_test_case_name( tc_name ) )
    {
        data::for_each_sample( ds, *this );
    }
#endif

    virtual test_unit* next() const
    {
        if( m_test_cases.empty() )
            return 0;

        test_unit* res = m_test_cases.front();
        m_test_cases.pop_front();

        return res;
    }

    // !! ?? variadics based implementation
#define TC_MAKE(z,arity,_)                                                          \
    template<BOOST_PP_ENUM_PARAMS(arity, typename Arg)>                             \
    void    operator()( BOOST_PP_ENUM_BINARY_PARAMS(arity, Arg, const& arg) ) const \
    {                                                                               \
        m_test_cases.push_back( new test_case( m_tc_name, m_tc_file, m_tc_line,     \
         boost::bind( &TestCase::template test_method<BOOST_PP_ENUM_PARAMS(arity,Arg)>, \
         BOOST_PP_ENUM_PARAMS(arity, arg) ) ) );                                    \
    }                                                                               \

    BOOST_PP_REPEAT_FROM_TO(1, 4, TC_MAKE, _)

private:
    // Data members
    std::string                     m_tc_name;
    const_string                    m_tc_file;
    std::size_t                     m_tc_line;
    mutable std::list<test_unit*>   m_test_cases;
};

//____________________________________________________________________________//

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
template<typename TestCase,typename DataSet>
test_case_gen<TestCase,DataSet>
make_test_case_gen( const_string tc_name, const_string tc_file, std::size_t tc_line, DataSet&& ds )
{
    return test_case_gen<TestCase,DataSet>( tc_name, tc_file, tc_line, std::forward<DataSet>(ds) );
}
#else
template<typename TestCase,typename DataSet>
test_case_gen<TestCase,DataSet>
make_test_case_gen( const_string tc_name, const_string tc_file, std::size_t tc_line, DataSet const& ds )
{
    return test_case_gen<TestCase,DataSet>( tc_name, tc_file, tc_line, ds );
}
#endif

//____________________________________________________________________________//

} // namespace ds_detail

// ************************************************************************** //
// **************             BOOST_DATA_TEST_CASE             ************** //
// ************************************************************************** //

#define BOOST_DATA_TEST_CASE_PARAM(r, _, i, param)  (BOOST_PP_CAT(Arg, i) const& param)
#define BOOST_DATA_TEST_CONTEXT(r, _, param)  << BOOST_STRINGIZE(param) << " = " << boost::test_tools::tt_detail::print_helper(param) << "; "

#define BOOST_DATA_TEST_CASE_PARAMS( params )                           \
    BOOST_PP_SEQ_ENUM(                                                  \
        BOOST_PP_SEQ_FOR_EACH_I(BOOST_DATA_TEST_CASE_PARAM, _, params)) \
/**/

#define BOOST_DATA_TEST_CASE_IMPL( arity, test_name, dataset, params )  \
struct test_name {                                                      \
    template<BOOST_PP_ENUM_PARAMS(arity, typename Arg)>                 \
    static void test_method( BOOST_DATA_TEST_CASE_PARAMS( params ) )    \
    {                                                                   \
        BOOST_TEST_CONTEXT( ""                                          \
            BOOST_PP_SEQ_FOR_EACH(BOOST_DATA_TEST_CONTEXT, _, params))  \
            _impl(BOOST_PP_SEQ_ENUM(params));                           \
    }                                                                   \
private:                                                                \
    template<BOOST_PP_ENUM_PARAMS(arity, typename Arg)>                 \
    static void _impl(BOOST_DATA_TEST_CASE_PARAMS( params ));           \
};                                                                      \
                                                                        \
BOOST_AUTO_TU_REGISTRAR( test_name )(                                   \
    boost::unit_test::data::ds_detail::make_test_case_gen<test_name>(   \
          BOOST_STRINGIZE( test_name ),                                 \
          __FILE__, __LINE__,                                           \
          boost::unit_test::data::make(dataset) ),                      \
    boost::unit_test::decorator::collector::instance() );               \
                                                                        \
    template<BOOST_PP_ENUM_PARAMS(arity, typename Arg)>                 \
    void test_name::_impl( BOOST_DATA_TEST_CASE_PARAMS( params ) )      \
/**/

#define BOOST_DATA_TEST_CASE_WITH_PARAMS( test_name, dataset, ... )     \
    BOOST_DATA_TEST_CASE_IMPL( BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),     \
                               test_name, dataset,                      \
                               BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__) )  \
/**/
#define BOOST_DATA_TEST_CASE_NO_PARAMS( test_name, dataset )            \
    BOOST_DATA_TEST_CASE_WITH_PARAMS( test_name, dataset, sample )      \
/**/

#if BOOST_PP_VARIADICS_MSVC

#define BOOST_DATA_TEST_CASE( ... )                                     \
    BOOST_PP_CAT(                                                       \
    BOOST_PP_IIF(BOOST_PP_EQUAL(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),2), \
                     BOOST_DATA_TEST_CASE_NO_PARAMS,                    \
                     BOOST_DATA_TEST_CASE_WITH_PARAMS) (__VA_ARGS__), ) \
/**/
#else

#define BOOST_DATA_TEST_CASE( ... )                                     \
    BOOST_PP_IIF(BOOST_PP_EQUAL(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),2), \
                     BOOST_DATA_TEST_CASE_NO_PARAMS,                    \
                     BOOST_DATA_TEST_CASE_WITH_PARAMS) (__VA_ARGS__)    \
/**/
#endif

} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_TEST_CASE_HPP_102211GER
