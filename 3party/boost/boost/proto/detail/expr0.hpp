///////////////////////////////////////////////////////////////////////////////
// expr.hpp
// Contains definition of expr\<\> class template.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PP_IS_ITERATING
#error Do not include this file directly
#endif

#define ARG_COUNT BOOST_PP_MAX(1, BOOST_PP_ITERATION())

    /// \brief Simplified representation of a node in an expression tree.
    ///
    /// \c proto::basic_expr\<\> is a node in an expression template tree. It
    /// is a container for its child sub-trees. It also serves as
    /// the terminal nodes of the tree.
    ///
    /// \c Tag is type that represents the operation encoded by
    ///             this expression. It is typically one of the structs
    ///             in the \c boost::proto::tag namespace, but it doesn't
    ///             have to be.
    ///
    /// \c Args is a type list representing the type of the children
    ///             of this expression. It is an instantiation of one
    ///             of \c proto::list1\<\>, \c proto::list2\<\>, etc. The
    ///             child types must all themselves be either \c expr\<\>
    ///             or <tt>proto::expr\<\>&</tt>. If \c Args is an
    ///             instantiation of \c proto::term\<\> then this
    ///             \c expr\<\> type represents a terminal expression;
    ///             the parameter to the \c proto::term\<\> template
    ///             represents the terminal's value type.
    ///
    /// \c Arity is an integral constant representing the number of child
    ///             nodes this node contains. If \c Arity is 0, then this
    ///             node is a terminal.
    ///
    /// \c proto::basic_expr\<\> is a valid Fusion random-access sequence, where
    /// the elements of the sequence are the child expressions.
    #ifdef BOOST_PROTO_DEFINE_TERMINAL
    template<typename Tag, typename Arg0>
    struct basic_expr<Tag, term<Arg0>, 0>
    #else
    template<typename Tag BOOST_PP_ENUM_TRAILING_PARAMS(ARG_COUNT, typename Arg)>
    struct basic_expr<Tag, BOOST_PP_CAT(list, BOOST_PP_ITERATION())<BOOST_PP_ENUM_PARAMS(ARG_COUNT, Arg)>, BOOST_PP_ITERATION() >
    #endif
    {
        typedef Tag proto_tag;
        BOOST_STATIC_CONSTANT(long, proto_arity_c = BOOST_PP_ITERATION());
        typedef mpl::long_<BOOST_PP_ITERATION() > proto_arity;
        typedef basic_expr proto_base_expr;
        #ifdef BOOST_PROTO_DEFINE_TERMINAL
        typedef term<Arg0> proto_args;
        #else
        typedef BOOST_PP_CAT(list, BOOST_PP_ITERATION())<BOOST_PP_ENUM_PARAMS(ARG_COUNT, Arg)> proto_args;
        #endif
        typedef basic_expr proto_grammar;
        typedef default_domain proto_domain;
        typedef default_generator proto_generator;
        typedef proto::tag::proto_expr fusion_tag;
        typedef basic_expr proto_derived_expr;
        typedef void proto_is_expr_; /**< INTERNAL ONLY */

        BOOST_PP_REPEAT(ARG_COUNT, BOOST_PROTO_CHILD, ~)
        BOOST_PP_REPEAT_FROM_TO(ARG_COUNT, BOOST_PROTO_MAX_ARITY, BOOST_PROTO_VOID, ~)

        /// \return *this
        ///
        basic_expr const &proto_base() const
        {
            return *this;
        }

        /// \overload
        ///
        basic_expr &proto_base()
        {
            return *this;
        }

    #ifdef BOOST_PROTO_DEFINE_TERMINAL
        /// \return A new \c expr\<\> object initialized with the specified
        /// arguments.
        ///
        template<typename A0>
        static basic_expr const make(A0 &a0)
        {
            return detail::make_terminal(a0, static_cast<basic_expr *>(0), static_cast<proto_args *>(0));
        }

        /// \overload
        ///
        template<typename A0>
        static basic_expr const make(A0 const &a0)
        {
            return detail::make_terminal(a0, static_cast<basic_expr *>(0), static_cast<proto_args *>(0));
        }
    #else
        /// \return A new \c expr\<\> object initialized with the specified
        /// arguments.
        ///
        template<BOOST_PP_ENUM_PARAMS(ARG_COUNT, typename A)>
        static basic_expr const make(BOOST_PP_ENUM_BINARY_PARAMS(ARG_COUNT, A, const &a))
        {
            basic_expr that = {BOOST_PP_ENUM_PARAMS(ARG_COUNT, a)};
            return that;
        }
    #endif

    #if 1 == BOOST_PP_ITERATION()
        /// If \c Tag is \c boost::proto::tag::address_of and \c proto_child0 is
        /// <tt>T&</tt>, then \c address_of_hack_type_ is <tt>T*</tt>.
        /// Otherwise, it is some undefined type.
        typedef typename detail::address_of_hack<Tag, proto_child0>::type address_of_hack_type_;

        /// \return The address of <tt>this->child0</tt> if \c Tag is
        /// \c boost::proto::tag::address_of. Otherwise, this function will
        /// fail to compile.
        ///
        /// \attention Proto overloads <tt>operator&</tt>, which means that
        /// proto-ified objects cannot have their addresses taken, unless we use
        /// the following hack to make \c &x implicitly convertible to \c X*.
        operator address_of_hack_type_() const
        {
            return boost::addressof(this->child0);
        }
    #else
        /// INTERNAL ONLY
        ///
        typedef detail::not_a_valid_type address_of_hack_type_;
    #endif
    };

    /// \brief Representation of a node in an expression tree.
    ///
    /// \c proto::expr\<\> is a node in an expression template tree. It
    /// is a container for its child sub-trees. It also serves as
    /// the terminal nodes of the tree.
    ///
    /// \c Tag is type that represents the operation encoded by
    ///             this expression. It is typically one of the structs
    ///             in the \c boost::proto::tag namespace, but it doesn't
    ///             have to be.
    ///
    /// \c Args is a type list representing the type of the children
    ///             of this expression. It is an instantiation of one
    ///             of \c proto::list1\<\>, \c proto::list2\<\>, etc. The
    ///             child types must all themselves be either \c expr\<\>
    ///             or <tt>proto::expr\<\>&</tt>. If \c Args is an
    ///             instantiation of \c proto::term\<\> then this
    ///             \c expr\<\> type represents a terminal expression;
    ///             the parameter to the \c proto::term\<\> template
    ///             represents the terminal's value type.
    ///
    /// \c Arity is an integral constant representing the number of child
    ///             nodes this node contains. If \c Arity is 0, then this
    ///             node is a terminal.
    ///
    /// \c proto::expr\<\> is a valid Fusion random-access sequence, where
    /// the elements of the sequence are the child expressions.
    #ifdef BOOST_PROTO_DEFINE_TERMINAL
    template<typename Tag, typename Arg0>
    struct expr<Tag, term<Arg0>, 0>
    #else
    template<typename Tag BOOST_PP_ENUM_TRAILING_PARAMS(ARG_COUNT, typename Arg)>
    struct expr<Tag, BOOST_PP_CAT(list, BOOST_PP_ITERATION())<BOOST_PP_ENUM_PARAMS(ARG_COUNT, Arg)>, BOOST_PP_ITERATION() >
    #endif
    {
        typedef Tag proto_tag;
        BOOST_STATIC_CONSTANT(long, proto_arity_c = BOOST_PP_ITERATION());
        typedef mpl::long_<BOOST_PP_ITERATION() > proto_arity;
        typedef expr proto_base_expr;
        #ifdef BOOST_PROTO_DEFINE_TERMINAL
        typedef term<Arg0> proto_args;
        #else
        typedef BOOST_PP_CAT(list, BOOST_PP_ITERATION())<BOOST_PP_ENUM_PARAMS(ARG_COUNT, Arg)> proto_args;
        #endif
        typedef basic_expr<Tag, proto_args, BOOST_PP_ITERATION() > proto_grammar;
        typedef default_domain proto_domain;
        typedef default_generator proto_generator;
        typedef proto::tag::proto_expr fusion_tag;
        typedef expr proto_derived_expr;
        typedef void proto_is_expr_; /**< INTERNAL ONLY */

        BOOST_PP_REPEAT(ARG_COUNT, BOOST_PROTO_CHILD, ~)
        BOOST_PP_REPEAT_FROM_TO(ARG_COUNT, BOOST_PROTO_MAX_ARITY, BOOST_PROTO_VOID, ~)

        /// \return *this
        ///
        expr const &proto_base() const
        {
            return *this;
        }

        /// \overload
        ///
        expr &proto_base()
        {
            return *this;
        }

    #ifdef BOOST_PROTO_DEFINE_TERMINAL
        /// \return A new \c expr\<\> object initialized with the specified
        /// arguments.
        ///
        template<typename A0>
        static expr const make(A0 &a0)
        {
            return detail::make_terminal(a0, static_cast<expr *>(0), static_cast<proto_args *>(0));
        }

        /// \overload
        ///
        template<typename A0>
        static expr const make(A0 const &a0)
        {
            return detail::make_terminal(a0, static_cast<expr *>(0), static_cast<proto_args *>(0));
        }
    #else
        /// \return A new \c expr\<\> object initialized with the specified
        /// arguments.
        ///
        template<BOOST_PP_ENUM_PARAMS(ARG_COUNT, typename A)>
        static expr const make(BOOST_PP_ENUM_BINARY_PARAMS(ARG_COUNT, A, const &a))
        {
            expr that = {BOOST_PP_ENUM_PARAMS(ARG_COUNT, a)};
            return that;
        }
    #endif

    #if 1 == BOOST_PP_ITERATION()
        /// If \c Tag is \c boost::proto::tag::address_of and \c proto_child0 is
        /// <tt>T&</tt>, then \c address_of_hack_type_ is <tt>T*</tt>.
        /// Otherwise, it is some undefined type.
        typedef typename detail::address_of_hack<Tag, proto_child0>::type address_of_hack_type_;

        /// \return The address of <tt>this->child0</tt> if \c Tag is
        /// \c boost::proto::tag::address_of. Otherwise, this function will
        /// fail to compile.
        ///
        /// \attention Proto overloads <tt>operator&</tt>, which means that
        /// proto-ified objects cannot have their addresses taken, unless we use
        /// the following hack to make \c &x implicitly convertible to \c X*.
        operator address_of_hack_type_() const
        {
            return boost::addressof(this->child0);
        }
    #else
        /// INTERNAL ONLY
        ///
        typedef detail::not_a_valid_type address_of_hack_type_;
    #endif

        /// Assignment
        ///
        /// \param a The rhs.
        /// \return A new \c expr\<\> node representing an assignment of \c that to \c *this.
        proto::expr<
            proto::tag::assign
          , list2<expr &, expr const &>
          , 2
        > const
        operator =(expr const &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, expr const &>
              , 2
            > that = {*this, a};
            return that;
        }

        /// Assignment
        ///
        /// \param a The rhs.
        /// \return A new \c expr\<\> node representing an assignment of \c a to \c *this.
        template<typename A>
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator =(A &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }

        /// \overload
        ///
        template<typename A>
        proto::expr<
            proto::tag::assign
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator =(A const &a) const
        {
            proto::expr<
                proto::tag::assign
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }

    #ifdef BOOST_PROTO_DEFINE_TERMINAL
        /// \overload
        ///
        template<typename A>
        proto::expr<
            proto::tag::assign
          , list2<expr &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator =(A &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }

        /// \overload
        ///
        template<typename A>
        proto::expr<
            proto::tag::assign
          , list2<expr &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator =(A const &a)
        {
            proto::expr<
                proto::tag::assign
              , list2<expr &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
    #endif

        /// Subscript
        ///
        /// \param a The rhs.
        /// \return A new \c expr\<\> node representing \c *this subscripted with \c a.
        template<typename A>
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator [](A &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }

        /// \overload
        ///
        template<typename A>
        proto::expr<
            proto::tag::subscript
          , list2<expr const &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator [](A const &a) const
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr const &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }

    #ifdef BOOST_PROTO_DEFINE_TERMINAL
        /// \overload
        ///
        template<typename A>
        proto::expr<
            proto::tag::subscript
          , list2<expr &, typename result_of::as_child<A>::type>
          , 2
        > const
        operator [](A &a)
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr &, typename result_of::as_child<A>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }

        /// \overload
        ///
        template<typename A>
        proto::expr<
            proto::tag::subscript
          , list2<expr &, typename result_of::as_child<A const>::type>
          , 2
        > const
        operator [](A const &a)
        {
            proto::expr<
                proto::tag::subscript
              , list2<expr &, typename result_of::as_child<A const>::type>
              , 2
            > that = {*this, proto::as_child(a)};
            return that;
        }
    #endif

        /// Encodes the return type of \c expr\<\>::operator(), for use with \c boost::result_of\<\>
        ///
        template<typename Sig>
        struct result
        {
            typedef typename result_of::funop<Sig, expr, default_domain>::type const type;
        };

        /// Function call
        ///
        /// \return A new \c expr\<\> node representing the function invocation of \c (*this)().
        proto::expr<proto::tag::function, list1<expr const &>, 1> const
        operator ()() const
        {
            proto::expr<proto::tag::function, list1<expr const &>, 1> that = {*this};
            return that;
        }

    #ifdef BOOST_PROTO_DEFINE_TERMINAL
        /// \overload
        ///
        proto::expr<proto::tag::function, list1<expr &>, 1> const
        operator ()()
        {
            proto::expr<proto::tag::function, list1<expr &>, 1> that = {*this};
            return that;
        }
    #endif

#define BOOST_PP_ITERATION_PARAMS_2 (3, (1, BOOST_PP_DEC(BOOST_PROTO_MAX_FUNCTION_CALL_ARITY), <boost/proto/detail/expr1.hpp>))
#include BOOST_PP_ITERATE()
    };

#undef ARG_COUNT
