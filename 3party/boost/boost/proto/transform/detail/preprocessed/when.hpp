    ///////////////////////////////////////////////////////////////////////////////
    /// \file when.hpp
    /// Definition of when transform.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R >
    struct when<Grammar, R()>
      : transform<when<Grammar, R()> >
    {
        typedef Grammar first;
        typedef R second();
        typedef typename Grammar::proto_grammar proto_grammar;
        
        
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef
                typename mpl::if_c<
                    is_callable<R>::value
                  , call<R()> 
                  , make<R()> 
                >::type
            which;
            typedef typename which::template impl<Expr, State, Data>::result_type result_type;
            
            
            
            
            
            
            
            
            
            
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                return typename which::template impl<Expr, State, Data>()(e, s, d);
            }
        };
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0>
    struct when<Grammar, R(A0)>
      : transform<when<Grammar, R(A0)> >
    {
        typedef Grammar first;
        typedef R second(A0);
        typedef typename Grammar::proto_grammar proto_grammar;
        
        
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef
                typename mpl::if_c<
                    is_callable<R>::value
                  , call<R(A0)> 
                  , make<R(A0)> 
                >::type
            which;
            typedef typename which::template impl<Expr, State, Data>::result_type result_type;
            
            
            
            
            
            
            
            
            
            
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                return typename which::template impl<Expr, State, Data>()(e, s, d);
            }
        };
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1>
    struct when<Grammar, R(A0 , A1)>
      : transform<when<Grammar, R(A0 , A1)> >
    {
        typedef Grammar first;
        typedef R second(A0 , A1);
        typedef typename Grammar::proto_grammar proto_grammar;
        
        
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef
                typename mpl::if_c<
                    is_callable<R>::value
                  , call<R(A0 , A1)> 
                  , make<R(A0 , A1)> 
                >::type
            which;
            typedef typename which::template impl<Expr, State, Data>::result_type result_type;
            
            
            
            
            
            
            
            
            
            
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                return typename which::template impl<Expr, State, Data>()(e, s, d);
            }
        };
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2>
    struct when<Grammar, R(A0 , A1 , A2)>
      : transform<when<Grammar, R(A0 , A1 , A2)> >
    {
        typedef Grammar first;
        typedef R second(A0 , A1 , A2);
        typedef typename Grammar::proto_grammar proto_grammar;
        
        
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef
                typename mpl::if_c<
                    is_callable<R>::value
                  , call<R(A0 , A1 , A2)> 
                  , make<R(A0 , A1 , A2)> 
                >::type
            which;
            typedef typename which::template impl<Expr, State, Data>::result_type result_type;
            
            
            
            
            
            
            
            
            
            
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                return typename which::template impl<Expr, State, Data>()(e, s, d);
            }
        };
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3>
    struct when<Grammar, R(A0 , A1 , A2 , A3)>
      : transform<when<Grammar, R(A0 , A1 , A2 , A3)> >
    {
        typedef Grammar first;
        typedef R second(A0 , A1 , A2 , A3);
        typedef typename Grammar::proto_grammar proto_grammar;
        
        
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef
                typename mpl::if_c<
                    is_callable<R>::value
                  , call<R(A0 , A1 , A2 , A3)> 
                  , make<R(A0 , A1 , A2 , A3)> 
                >::type
            which;
            typedef typename which::template impl<Expr, State, Data>::result_type result_type;
            
            
            
            
            
            
            
            
            
            
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                return typename which::template impl<Expr, State, Data>()(e, s, d);
            }
        };
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4)>
      : transform<when<Grammar, R(A0 , A1 , A2 , A3 , A4)> >
    {
        typedef Grammar first;
        typedef R second(A0 , A1 , A2 , A3 , A4);
        typedef typename Grammar::proto_grammar proto_grammar;
        
        
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef
                typename mpl::if_c<
                    is_callable<R>::value
                  , call<R(A0 , A1 , A2 , A3 , A4)> 
                  , make<R(A0 , A1 , A2 , A3 , A4)> 
                >::type
            which;
            typedef typename which::template impl<Expr, State, Data>::result_type result_type;
            
            
            
            
            
            
            
            
            
            
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                return typename which::template impl<Expr, State, Data>()(e, s, d);
            }
        };
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5)>
      : transform<when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5)> >
    {
        typedef Grammar first;
        typedef R second(A0 , A1 , A2 , A3 , A4 , A5);
        typedef typename Grammar::proto_grammar proto_grammar;
        
        
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef
                typename mpl::if_c<
                    is_callable<R>::value
                  , call<R(A0 , A1 , A2 , A3 , A4 , A5)> 
                  , make<R(A0 , A1 , A2 , A3 , A4 , A5)> 
                >::type
            which;
            typedef typename which::template impl<Expr, State, Data>::result_type result_type;
            
            
            
            
            
            
            
            
            
            
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                return typename which::template impl<Expr, State, Data>()(e, s, d);
            }
        };
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6)>
      : transform<when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6)> >
    {
        typedef Grammar first;
        typedef R second(A0 , A1 , A2 , A3 , A4 , A5 , A6);
        typedef typename Grammar::proto_grammar proto_grammar;
        
        
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef
                typename mpl::if_c<
                    is_callable<R>::value
                  , call<R(A0 , A1 , A2 , A3 , A4 , A5 , A6)> 
                  , make<R(A0 , A1 , A2 , A3 , A4 , A5 , A6)> 
                >::type
            which;
            typedef typename which::template impl<Expr, State, Data>::result_type result_type;
            
            
            
            
            
            
            
            
            
            
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                return typename which::template impl<Expr, State, Data>()(e, s, d);
            }
        };
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)>
      : transform<when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)> >
    {
        typedef Grammar first;
        typedef R second(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7);
        typedef typename Grammar::proto_grammar proto_grammar;
        
        
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef
                typename mpl::if_c<
                    is_callable<R>::value
                  , call<R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)> 
                  , make<R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)> 
                >::type
            which;
            typedef typename which::template impl<Expr, State, Data>::result_type result_type;
            
            
            
            
            
            
            
            
            
            
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                return typename which::template impl<Expr, State, Data>()(e, s, d);
            }
        };
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)>
      : transform<when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)> >
    {
        typedef Grammar first;
        typedef R second(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8);
        typedef typename Grammar::proto_grammar proto_grammar;
        
        
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef
                typename mpl::if_c<
                    is_callable<R>::value
                  , call<R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)> 
                  , make<R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)> 
                >::type
            which;
            typedef typename which::template impl<Expr, State, Data>::result_type result_type;
            
            
            
            
            
            
            
            
            
            
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                return typename which::template impl<Expr, State, Data>()(e, s, d);
            }
        };
    };
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    template<typename Grammar, typename R , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)>
      : transform<when<Grammar, R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)> >
    {
        typedef Grammar first;
        typedef R second(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9);
        typedef typename Grammar::proto_grammar proto_grammar;
        
        
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            
            typedef
                typename mpl::if_c<
                    is_callable<R>::value
                  , call<R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)> 
                  , make<R(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)> 
                >::type
            which;
            typedef typename which::template impl<Expr, State, Data>::result_type result_type;
            
            
            
            
            
            
            
            
            
            
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                return typename which::template impl<Expr, State, Data>()(e, s, d);
            }
        };
    };
