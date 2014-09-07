/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(SPIRIT_EXPECT_MARCH_16_2012_1024PM)
#define SPIRIT_EXPECT_MARCH_16_2012_1024PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/x3/support/context.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace boost { namespace spirit { namespace x3
{
    template <typename Iterator>
    struct expectation_failure : std::runtime_error
    {
    public:

        expectation_failure(Iterator where, std::string const& which)
          : std::runtime_error("boost::spirit::x3::expectation_failure")
          , where_(where), which_(which)
        {}
        ~expectation_failure() throw() {}
        
        std::string which() const { return which_; }
        Iterator const& where() const { return where_; }
        
    private:

        Iterator where_;
        std::string which_;
    };

    template <typename Subject>
    struct expect_directive : unary_parser<Subject, expect_directive<Subject>>
    {
        typedef unary_parser<Subject, expect_directive<Subject> > base_type;
        static bool const is_pass_through_unary = true;

        expect_directive(Subject const& subject)
          : base_type(subject) {}

        template <typename Iterator, typename Context
          , typename RContext, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context const& context, RContext& rcontext, Attribute& attr) const
        {
            bool r = this->subject.parse(first, last, context, rcontext, attr);

            if (!r)
            {
                boost::throw_exception(
                    expectation_failure<Iterator>(
                        first, what(this->subject)));
            }
            return r;
        }
    };

    struct expect_gen
    {
        template <typename Subject>
        expect_directive<typename extension::as_parser<Subject>::value_type>
        operator[](Subject const& subject) const
        {
            return {as_parser(subject)};
        }
    };

    expect_gen const expect = expect_gen();
}}}

#endif
