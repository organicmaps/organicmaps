/*=============================================================================
    Copyright (c) 2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_TESTING_JUNE_05_2014_00422PM)
#define BOOST_SPIRIT_X3_TESTING_JUNE_05_2014_00422PM

namespace boost { namespace spirit { namespace x3 { namespace testing
{
    ////////////////////////////////////////////////////////////////////////////
    //
    //  Test utility
    //
    //  The test function accepts a file loaded in memory. The 'test_file'
    //  range param points to the data contained in the file. This file
    //  contains two parts.
    //
    //      1) The source input for testing
    //      2) The expected result.
    //
    //  The first part of the file is sent to the generator function
    //  'gen' which returns a string. This generated string is then compared
    //  to the contents of the second (expected result) part.
    //
    //  The second part is demarcated by the string parameter 'demarcation'
    //  which defaults to "<**expected**>". The expected template may include
    //  embedded regular expressions marked-up within re_prefix and re_suffix
    //  parameter tags. For example, given the default RE markup ("<%" and
    //  "%>"), this template:
    //
    //      <%[0-9]+%>
    //
    //  will match any integer in the source input being tested. The function
    //  will return the first non-matching position. The flag full_match
    //  indicates a full match. It is possible for returned pos to be
    //  at the end of in (in.end()) while still returning full_match ==
    //  false. In that case, we have a partial match.
    //
    //  Here's an example of a test file:
    //
    //      Hello World, I am Joel. This is a test.
    //
    //      <**expected**>
    //      Hello World, I am <%[a-zA-Z]+%>. This is a test.
    //
    ////////////////////////////////////////////////////////////////////////////

    template <typename Iterator>
    struct test_result
    {
        Iterator pos;
        bool full_match;
    };

    template <typename Range, typename F>
    test_result<typename Range::const_iterator>
    test(
        Range test_file
      , F gen
      , char const* demarcation = "<**expected**>"
      , char const* re_prefix = "<%"
      , char const* re_suffix = "%>"
   );

}}}}

#endif
