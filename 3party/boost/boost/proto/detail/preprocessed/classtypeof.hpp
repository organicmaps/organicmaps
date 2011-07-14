    ///////////////////////////////////////////////////////////////////////////////
    // classtypeof.hpp
    // Contains specializations of the classtypeof\<\> class template.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    template<typename T, typename U >
    struct classtypeof<T (U::*)()>
    {
        typedef U type;
    };
    template<typename T, typename U >
    struct classtypeof<T (U::*)() const>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0>
    struct classtypeof<T (U::*)(A0)>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0>
    struct classtypeof<T (U::*)(A0) const>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1>
    struct classtypeof<T (U::*)(A0 , A1)>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1>
    struct classtypeof<T (U::*)(A0 , A1) const>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2>
    struct classtypeof<T (U::*)(A0 , A1 , A2)>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2>
    struct classtypeof<T (U::*)(A0 , A1 , A2) const>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2 , typename A3>
    struct classtypeof<T (U::*)(A0 , A1 , A2 , A3)>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2 , typename A3>
    struct classtypeof<T (U::*)(A0 , A1 , A2 , A3) const>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct classtypeof<T (U::*)(A0 , A1 , A2 , A3 , A4)>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2 , typename A3 , typename A4>
    struct classtypeof<T (U::*)(A0 , A1 , A2 , A3 , A4) const>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct classtypeof<T (U::*)(A0 , A1 , A2 , A3 , A4 , A5)>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5>
    struct classtypeof<T (U::*)(A0 , A1 , A2 , A3 , A4 , A5) const>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct classtypeof<T (U::*)(A0 , A1 , A2 , A3 , A4 , A5 , A6)>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6>
    struct classtypeof<T (U::*)(A0 , A1 , A2 , A3 , A4 , A5 , A6) const>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct classtypeof<T (U::*)(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7)>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7>
    struct classtypeof<T (U::*)(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7) const>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct classtypeof<T (U::*)(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8)>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8>
    struct classtypeof<T (U::*)(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8) const>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct classtypeof<T (U::*)(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9)>
    {
        typedef U type;
    };
    template<typename T, typename U , typename A0 , typename A1 , typename A2 , typename A3 , typename A4 , typename A5 , typename A6 , typename A7 , typename A8 , typename A9>
    struct classtypeof<T (U::*)(A0 , A1 , A2 , A3 , A4 , A5 , A6 , A7 , A8 , A9) const>
    {
        typedef U type;
    };
