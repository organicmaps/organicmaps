/*=============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_CORE_DOMAIN_HPP
#define BOOST_PHOENIX_CORE_DOMAIN_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/proto/domain.hpp>

namespace boost { namespace phoenix
{
    template <typename Expr>
    struct actor;
    
    struct meta_grammar;
    
    struct phoenix_domain
        : proto::domain<
            proto::pod_generator<actor>
          , meta_grammar
          , proto::basic_default_domain
        >
    {
        template <typename T>
        struct as_child
            : as_expr<T>
        {};
    };
}}

#endif
