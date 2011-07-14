    ///////////////////////////////////////////////////////////////////////////////
    /// \file lazy.hpp
    /// Contains definition of the lazy<> transform.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    
    
    
    
    
    
    
    
    template<typename Object >
    struct lazy<Object()>
      : transform<lazy<Object()> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename make<Object>::template impl<Expr, State, Data>::result_type
                ()
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    
    
    
    
    
    template<typename Object , typename A0>
    struct lazy<Object(A0)>
      : transform<lazy<Object(A0)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename make<Object>::template impl<Expr, State, Data>::result_type
                (A0)
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    
    
    
    
    
    template<typename Object , typename A0 , typename A1>
    struct lazy<Object(A0 , A1)>
      : transform<lazy<Object(A0 , A1)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename make<Object>::template impl<Expr, State, Data>::result_type
                (A0 , A1)
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2>
    struct lazy<Object(A0 , A1 , A2)>
      : transform<lazy<Object(A0 , A1 , A2)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename make<Object>::template impl<Expr, State, Data>::result_type
                (A0 , A1 , A2)
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3>
    struct lazy<Object(A0 , A1 , A2 , A3)>
      : transform<lazy<Object(A0 , A1 , A2 , A3)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename make<Object>::template impl<Expr, State, Data>::result_type
                (A0 , A1 , A2 , A3)
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct lazy<Object(A0 , A1 , A2 , A3 , A4)>
      : transform<lazy<Object(A0 , A1 , A2 , A3 , A4)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename make<Object>::template impl<Expr, State, Data>::result_type
                (A0 , A1 , A2 , A3 , A4)
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct lazy<Object(A0 , A1 , A2 , A3 , A4 , A5)>
      : transform<lazy<Object(A0 , A1 , A2 , A3 , A4 , A5)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename make<Object>::template impl<Expr, State, Data>::result_type
                (A0 , A1 , A2 , A3 , A4 , A5)
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct lazy<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6)>
      : transform<lazy<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename make<Object>::template impl<Expr, State, Data>::result_type
                (A0 , A1 , A2 , A3 , A4 , A5 , A6)
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct lazy<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)>
      : transform<lazy<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename make<Object>::template impl<Expr, State, Data>::result_type
                (A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct lazy<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)>
      : transform<lazy<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename make<Object>::template impl<Expr, State, Data>::result_type
                (A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)
            >::template impl<Expr, State, Data>
        {};
    };
    
    
    
    
    
    
    
    
    template<typename Object , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct lazy<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)>
      : transform<lazy<Object(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : call<
                typename make<Object>::template impl<Expr, State, Data>::result_type
                (A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)
            >::template impl<Expr, State, Data>
        {};
    };
