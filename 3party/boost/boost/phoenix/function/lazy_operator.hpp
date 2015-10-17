////////////////////////////////////////////////////////////////////////////
// lazy operator.hpp
//
// Build lazy operations for Phoenix equivalents for FC++
//
// These are equivalents of the Boost FC++ functoids in operator.hpp
//
// Implemented so far:
//
// make_pair
// plus minus multiplies divides modulus
// negate equal not_equal greater less
// greater_equal less_equal logical_and logical_or
// logical_not min max inc dec
//
// These are not from the FC++ operator.hpp but were made for testing purposes.
//
// identity (renamed id)
// sin
//
// These are now being modified to use boost::phoenix::function
// so that they are available for use as arguments.
// Types are being defined in capitals e.g. Id id;
////////////////////////////////////////////////////////////////////////////
/*=============================================================================
    Copyright (c) 2000-2003 Brian McNamara and Yannis Smaragdakis
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2015 John Fletcher

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/


#ifndef BOOST_PHOENIX_FUNCTION_LAZY_OPERATOR
#define BOOST_PHOENIX_FUNCTION_LAZY_OPERATOR

#include <cmath>
#include <cstdlib>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/function.hpp>

namespace boost {

  namespace phoenix {

//////////////////////////////////////////////////////////////////////
// a_unique_type_for_nil
//////////////////////////////////////////////////////////////////////

// This may need to be moved elsewhere to define reuser.
   struct a_unique_type_for_nil {
     bool operator==( a_unique_type_for_nil ) const { return true; }
     bool operator< ( a_unique_type_for_nil ) const { return false; }
     typedef a_unique_type_for_nil value_type;
   };
    // This maybe put into a namespace.
   a_unique_type_for_nil NIL;

//////////////////////////////////////////////////////////////////////
// lazy_exception - renamed from fcpp_exception.
//////////////////////////////////////////////////////////////////////

#ifndef BOOST_PHOENIX_NO_LAZY_EXCEPTIONS
   struct lazy_exception : public std::exception {
       const char* s;
       lazy_exception( const char* ss ) : s(ss) {}
       const char* what() const throw() { return s; }
   };
#endif

//////////////////////////////////////////////////////////////////////

   // in ref_count.hpp in BoostFC++
   typedef unsigned int RefCountType;

    namespace impl {

      struct Id
      {
        template <typename Sig>
        struct result;

        template <typename This, typename A0>
        struct result<This(A0)>
           : boost::remove_reference<A0>
        {};

        template <typename A0>
        A0 operator()(A0 const & a0) const
        {
            return a0;
        }

      };


    }

    //BOOST_PHOENIX_ADAPT_CALLABLE(id, impl::id, 1)
    typedef boost::phoenix::function<impl::Id> Id;
    Id id;

#ifdef BOOST_RESULT_OF_USE_TR1
    // Experiment following examples in
    // phoenix/stl/container/container.hpp

    namespace result_of {

      template <
          typename Arg1
        , typename Arg2
      >
      class make_pair
      {
      public:
        typedef typename boost::remove_reference<Arg1>::type Arg1Type;
        typedef typename boost::remove_reference<Arg2>::type Arg2Type;
        typedef std::pair<Arg1Type,Arg2Type> type;
        typedef std::pair<Arg1Type,Arg2Type> result_type;
      };
    }
#endif

  namespace impl
  {

    struct make_pair {


#ifdef BOOST_RESULT_OF_USE_TR1
       template <typename Sig>
       struct result;
       // This fails with -O2 unless refs are removed from A1 and A2.
       template <typename This, typename A0, typename A1>
       struct result<This(A0, A1)>
       {
         typedef typename result_of::make_pair<A0,A1>::type type;
       };
#else
       template <typename Sig>
       struct result;

       template <typename This, typename A0, typename A1>
       struct result<This(A0, A1)>
         : boost::remove_reference<std::pair<A0, A1> >
       {};

#endif


       template <typename A0, typename A1>
#ifdef BOOST_RESULT_OF_USE_TR1
       typename result<make_pair(A0,A1)>::type
#else
       std::pair<A0, A1>
#endif
       operator()(A0 const & a0, A1 const & a1) const
       {
          return std::make_pair(a0,a1);
       }

    };
  }

BOOST_PHOENIX_ADAPT_CALLABLE(make_pair, impl::make_pair, 2)

  namespace impl
  {

    // For now I will leave the return type deduction as it is.
    // I want to look at bringing in the sort of type deduction for
    // mixed types which I have in FC++.
    // Also I could look at the case where one of the arguments is
    // another functor or a Phoenix placeholder.
    struct Plus
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
             : boost::remove_reference<A0>
        {};

        template <typename This, typename A0, typename A1, typename A2>
        struct result<This(A0, A1, A2)>
             : boost::remove_reference<A0>
        {};

        template <typename A0, typename A1>
        A0 operator()(A0 const & a0, A1 const & a1) const
        {
          //A0 res = a0 + a1;
          //return res;
          return a0 + a1;
        }

        template <typename A0, typename A1, typename A2>
        A0 operator()(A0 const & a0, A1 const & a1, A2 const & a2) const
        {
            return a0 + a1 + a2;
        }
    };

    struct Minus
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
           : boost::remove_reference<A0>
        {};

        template <typename A0, typename A1>
        A0 operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 - a1;
        }

    };

    struct multiplies
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
           : boost::remove_reference<A0>
        {};

        template <typename A0, typename A1>
        A0 operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 * a1;
        }

    };

    struct divides
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
           : boost::remove_reference<A0>
        {};

        template <typename A0, typename A1>
        A0 operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 / a1;
        }

    };

    struct modulus
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
          : boost::remove_reference<A0>
        {};

        template <typename A0, typename A1>
        A0 operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 % a1;
        }

    };

    struct negate
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0>
        struct result<This(A0)>
           : boost::remove_reference<A0>
        {};

        template <typename A0>
        A0 operator()(A0 const & a0) const
        {
            return -a0;
        }
    };

    struct equal
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 == a1;
        }
    };

    struct not_equal
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 != a1;
        }
    };

    struct greater
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 > a1;
        }
    };

    struct less
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 < a1;
        }
    };

    struct greater_equal
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 >= a1;
        }
    };

    struct less_equal
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 <= a1;
        }
    };

    struct logical_and
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 && a1;
        }
    };

    struct logical_or
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0,A1)>
        {
            typedef bool type;
        };

        template <typename A0, typename A1>
        bool operator()(A0 const & a0, A1 const & a1) const
        {
            return a0 || a1;
        }
    };

    struct logical_not
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0>
        struct result<This(A0)>
        {
             typedef bool type;
        };

        template <typename A0>
        bool operator()(A0 const & a0) const
        {
            return !a0;
        }
    };

    struct min
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
          : boost::remove_reference<A0>
        {};

        template <typename A0, typename A1>
        A0 operator()(A0 const & a0, A1 const & a1) const
        {
           if ( a0 < a1 ) return a0; else return a1;
        }

    };

    struct max
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0, typename A1>
        struct result<This(A0, A1)>
          : boost::remove_reference<A0>
        {};

        template <typename A0, typename A1>
        A0 operator()(A0 const & a0, A1 const & a1) const
        {
           if ( a0 < a1 ) return a1; else return a0;
        }

    };

    struct Inc
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0>
        struct result<This(A0)>
           : boost::remove_reference<A0>
        {};

        template <typename A0>
        A0 operator()(A0 const & a0) const
        {
            return a0 + 1;
        }

    };

    struct Dec
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0>
        struct result<This(A0)>
           : boost::remove_reference<A0>
        {};

        template <typename A0>
        A0 operator()(A0 const & a0) const
        {
            return a0 - 1;
        }

    };

    struct Sin
    {
        template <typename Sig>
        struct result;

        template <typename This, typename A0>
        struct result<This(A0)>
           : boost::remove_reference<A0>
        {};

        template <typename A0>
        A0 operator()(A0 const & a0) const
        {
          return std::sin(a0);
        }

    };

    // Example of templated struct.
    // How do I make it callable?
    template <typename Result>
    struct what {

      typedef Result result_type;

      Result operator()(Result const & r) const
      {
        return r;
      }
      // what is not complete - error.
      //static boost::function1<Result,Result> res = what<Result>();
    };

    template <typename Result>
    struct what0 {

      typedef Result result_type;

      Result operator()() const
      {
        return Result(100);
      }

    };

      template <class Result, class F>
      class MonomorphicWrapper0 /* : public c_fun_type<Res> */
      {
          F f;
      public:
          typedef Result result_type;
          MonomorphicWrapper0( const F& g ) : f(g) {}
             Result operator()() const {
             return f();
          }
      };

      /* I need the equivalent of this
      template <class Res, class F>
      full0<impl::XMonomorphicWrapper0<Res,F> > monomorphize0( const F& f )
      {
         return make_full0( impl::XMonomorphicWrapper0<Res,F>( f ) );
      }*/


    // boost::function0<int> res = MonomorphicWrapper0<int,F>(f);


      template <class Res, class F>
      boost::function<Res()> monomorphize0( const F& f )
      {
        boost::function0<Res> ff =  MonomorphicWrapper0<Res,F>( f );
        //BOOST_PHOENIX_ADAPT_FUNCTION_NULLARY(Res,fres,ff)
        return ff;
      }

    // This is C++1y
    //template <typename Result>
    //static boost::function1<Result,Result> res = what<Result>();


  }
    /////////////////////////////////////////////////////////
    // Look at this. How to use Phoenix with a templated
    // struct. First adapt with boost::function and then
    // convert that to Phoenix!!
    // I have not found out how to do it directly.
    /////////////////////////////////////////////////////////
boost::function1<int, int > what_int = impl::what<int>();
typedef boost::function1<int,int> fun1_int_int;
typedef boost::function0<int> fun0_int;
boost::function0<int> what0_int = impl::what0<int>();
BOOST_PHOENIX_ADAPT_FUNCTION(int,what,what_int,1)
BOOST_PHOENIX_ADAPT_FUNCTION_NULLARY(int,what0,what0_int)
// And this shows how to make them into argument callable functions.
typedef boost::phoenix::function<fun1_int_int> What_arg;
typedef boost::phoenix::function<fun0_int> What0_arg;
What_arg what_arg(what_int);
What0_arg what0_arg(what0_int);

//BOOST_PHOENIX_ADAPT_CALLABLE(plus, impl::plus, 2)
//BOOST_PHOENIX_ADAPT_CALLABLE(plus, impl::plus, 3)
//BOOST_PHOENIX_ADAPT_CALLABLE(minus, impl::minus, 2)
BOOST_PHOENIX_ADAPT_CALLABLE(multiplies, impl::multiplies, 2)
BOOST_PHOENIX_ADAPT_CALLABLE(divides, impl::divides, 2)
BOOST_PHOENIX_ADAPT_CALLABLE(modulus, impl::modulus, 2)
BOOST_PHOENIX_ADAPT_CALLABLE(negate, impl::negate, 1)
BOOST_PHOENIX_ADAPT_CALLABLE(equal, impl::equal, 2)
BOOST_PHOENIX_ADAPT_CALLABLE(not_equal, impl::not_equal, 2)
BOOST_PHOENIX_ADAPT_CALLABLE(greater, impl::greater, 2)
BOOST_PHOENIX_ADAPT_CALLABLE(less, impl::less, 2)
BOOST_PHOENIX_ADAPT_CALLABLE(greater_equal, impl::greater_equal, 2)
BOOST_PHOENIX_ADAPT_CALLABLE(less_equal, impl::less_equal, 2)
BOOST_PHOENIX_ADAPT_CALLABLE(logical_and, impl::logical_and, 2)
BOOST_PHOENIX_ADAPT_CALLABLE(logical_or, impl::logical_or, 2)
BOOST_PHOENIX_ADAPT_CALLABLE(logical_not, impl::logical_not, 1)
BOOST_PHOENIX_ADAPT_CALLABLE(min, impl::min, 2)
BOOST_PHOENIX_ADAPT_CALLABLE(max, impl::max, 2)
//BOOST_PHOENIX_ADAPT_CALLABLE(inc, impl::inc, 1)
//BOOST_PHOENIX_ADAPT_CALLABLE(dec, impl::dec, 1)
//BOOST_PHOENIX_ADAPT_CALLABLE(sin, impl::sin, 1)

// To use these as arguments they have to be defined like this.
    typedef boost::phoenix::function<impl::Plus> Plus;
    typedef boost::phoenix::function<impl::Minus> Minus;
    typedef boost::phoenix::function<impl::Inc> Inc;
    typedef boost::phoenix::function<impl::Dec> Dec;
    typedef boost::phoenix::function<impl::Sin> Sin;
    Plus plus;
    Minus minus;
    Inc inc;
    Dec dec;
    Sin sin;
}

}




#endif
