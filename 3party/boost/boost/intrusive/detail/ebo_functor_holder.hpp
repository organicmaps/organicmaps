/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Joaquin M Lopez Munoz  2006-2013
// (C) Copyright Ion Gaztanaga          2014-2014
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_DETAIL_EBO_HOLDER_HPP
#define BOOST_INTRUSIVE_DETAIL_EBO_HOLDER_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

namespace boost {
namespace intrusive {
namespace detail {

#if defined(BOOST_MSVC) || defined(__BORLANDC_)
#define BOOST_INTRUSIVE_TT_DECL __cdecl
#else
#define BOOST_INTRUSIVE_TT_DECL
#endif

#if defined(_MSC_EXTENSIONS) && !defined(__BORLAND__) && !defined(_WIN64) && !defined(_M_ARM) && !defined(UNDER_CE)
#define BOOST_INTRUSIVE_TT_TEST_MSC_FUNC_SIGS
#endif

template <typename T>
struct is_unary_or_binary_function_impl
{  static const bool value = false; };

// see boost ticket #4094
// avoid duplicate definitions of is_unary_or_binary_function_impl
#ifndef BOOST_INTRUSIVE_TT_TEST_MSC_FUNC_SIGS

template <typename R>
struct is_unary_or_binary_function_impl<R (*)()>
{  static const bool value = true;  };

template <typename R>
struct is_unary_or_binary_function_impl<R (*)(...)>
{  static const bool value = true;  };

#else // BOOST_INTRUSIVE_TT_TEST_MSC_FUNC_SIGS

template <typename R>
struct is_unary_or_binary_function_impl<R (__stdcall*)()>
{  static const bool value = true;  };

#ifndef _MANAGED

template <typename R>
struct is_unary_or_binary_function_impl<R (__fastcall*)()>
{  static const bool value = true;  };

#endif

template <typename R>
struct is_unary_or_binary_function_impl<R (__cdecl*)()>
{  static const bool value = true;  };

template <typename R>
struct is_unary_or_binary_function_impl<R (__cdecl*)(...)>
{  static const bool value = true;  };

#endif

// see boost ticket #4094
// avoid duplicate definitions of is_unary_or_binary_function_impl
#ifndef BOOST_INTRUSIVE_TT_TEST_MSC_FUNC_SIGS

template <typename R, class T0>
struct is_unary_or_binary_function_impl<R (*)(T0)>
{  static const bool value = true;  };

template <typename R, class T0>
struct is_unary_or_binary_function_impl<R (*)(T0...)>
{  static const bool value = true;  };

#else // BOOST_INTRUSIVE_TT_TEST_MSC_FUNC_SIGS

template <typename R, class T0>
struct is_unary_or_binary_function_impl<R (__stdcall*)(T0)>
{  static const bool value = true;  };

#ifndef _MANAGED

template <typename R, class T0>
struct is_unary_or_binary_function_impl<R (__fastcall*)(T0)>
{  static const bool value = true;  };

#endif

template <typename R, class T0>
struct is_unary_or_binary_function_impl<R (__cdecl*)(T0)>
{  static const bool value = true;  };

template <typename R, class T0>
struct is_unary_or_binary_function_impl<R (__cdecl*)(T0...)>
{  static const bool value = true;  };

#endif

// see boost ticket #4094
// avoid duplicate definitions of is_unary_or_binary_function_impl
#ifndef BOOST_INTRUSIVE_TT_TEST_MSC_FUNC_SIGS

template <typename R, class T0, class T1>
struct is_unary_or_binary_function_impl<R (*)(T0, T1)>
{  static const bool value = true;  };

template <typename R, class T0, class T1>
struct is_unary_or_binary_function_impl<R (*)(T0, T1...)>
{  static const bool value = true;  };

#else // BOOST_INTRUSIVE_TT_TEST_MSC_FUNC_SIGS

template <typename R, class T0, class T1>
struct is_unary_or_binary_function_impl<R (__stdcall*)(T0, T1)>
{  static const bool value = true;  };

#ifndef _MANAGED

template <typename R, class T0, class T1>
struct is_unary_or_binary_function_impl<R (__fastcall*)(T0, T1)>
{  static const bool value = true;  };

#endif

template <typename R, class T0, class T1>
struct is_unary_or_binary_function_impl<R (__cdecl*)(T0, T1)>
{  static const bool value = true;  };

template <typename R, class T0, class T1>
struct is_unary_or_binary_function_impl<R (__cdecl*)(T0, T1...)>
{  static const bool value = true;  };
#endif

template <typename T>
struct is_unary_or_binary_function_impl<T&>
{  static const bool value = false; };

template<typename T>
struct is_unary_or_binary_function : is_unary_or_binary_function_impl<T>
{};

template<typename T, bool IsEmpty = true>
class ebo_functor_holder_impl
{
   public:
   ebo_functor_holder_impl()
   {}
   ebo_functor_holder_impl(const T& t)
      :  t_(t)
   {}
   template<class Arg1, class Arg2>
   ebo_functor_holder_impl(const Arg1& arg1, const Arg2& arg2)
      :  t_(arg1, arg2)
   {}

   T&       get(){return t_;}
   const T& get()const{return t_;}

   private:
   T t_;
};

template<typename T>
class ebo_functor_holder_impl<T, false>
   :  public T
{
   public:
   ebo_functor_holder_impl()
   {}
   explicit ebo_functor_holder_impl(const T& t)
      :  T(t)
   {}
   template<class Arg1, class Arg2>
   ebo_functor_holder_impl(const Arg1& arg1, const Arg2& arg2)
      :  T(arg1, arg2)
   {}

   T&       get(){return *this;}
   const T& get()const{return *this;}
};

template<typename T>
class ebo_functor_holder
   :  public ebo_functor_holder_impl<T, is_unary_or_binary_function<T>::value>
{
   private:
   typedef ebo_functor_holder_impl<T, is_unary_or_binary_function<T>::value> super;

   public:
   typedef T functor_type;
   ebo_functor_holder(){}
   explicit ebo_functor_holder(const T& t)
      :  super(t)
   {}

   template<class Arg1, class Arg2>
   ebo_functor_holder(const Arg1& arg1, const Arg2& arg2)
      :  super(arg1, arg2)
   {}

   ebo_functor_holder& operator=(const ebo_functor_holder& x)
   {
      this->get()=x.get();
      return *this;
   }
};


}  //namespace detail {
}  //namespace intrusive {
}  //namespace boost {

#endif   //#ifndef BOOST_INTRUSIVE_DETAIL_EBO_HOLDER_HPP
