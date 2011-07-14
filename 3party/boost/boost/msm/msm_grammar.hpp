// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_GRAMMAR_H
#define BOOST_MSM_GRAMMAR_H

#include <boost/msm/common.hpp>


namespace boost { namespace msm
{
// base grammar for all of msm's proto-based grammars
struct basic_grammar : proto::_
{};

// Forward-declare an expression wrapper
template<typename Expr>
struct msm_terminal;

struct msm_domain
    : proto::domain< proto::generator<msm_terminal>, basic_grammar >
{};

template<typename Expr>
struct msm_terminal
    : proto::extends<Expr, msm_terminal<Expr>, msm_domain>
{
    typedef
        proto::extends<Expr, msm_terminal<Expr>, msm_domain>
        base_type;
    // Needs a constructor
    msm_terminal(Expr const &e = Expr())
        : base_type(e)
    {}
};

} } // boost::msm
#endif //BOOST_MSM_GRAMMAR_H

