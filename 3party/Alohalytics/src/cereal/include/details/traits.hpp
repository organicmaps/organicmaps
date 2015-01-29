/*! \file traits.hpp
    \brief Internal type trait support
    \ingroup Internal */
/*
  Copyright (c) 2014, Randolph Voorhies, Shane Grant
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of cereal nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL RANDOLPH VOORHIES OR SHANE GRANT BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef CEREAL_DETAILS_TRAITS_HPP_
#define CEREAL_DETAILS_TRAITS_HPP_

#ifndef __clang__
#if (__GNUC__ == 4 && __GNUC_MINOR__ <= 7)
#define CEREAL_OLDER_GCC
#endif // gcc 4.7 or earlier
#endif // __clang__

#include <type_traits>
#include <typeindex>

#include "../access.hpp"

namespace cereal
{
  namespace traits
  {
    typedef std::true_type yes;
    typedef std::false_type no;

    namespace detail
    {
      //! Used to delay a static_assert until template instantiation
      template <class T>
      struct delay_static_assert : std::false_type {};

      #ifdef CEREAL_OLDER_GCC // when VS supports better SFINAE, we can use this as the default
      template<typename> struct Void { typedef void type; };
      #endif // CEREAL_OLDER_GCC
    } // namespace detail

    //! Creates a test for whether a non const member function exists
    /*! This creates a class derived from std::integral_constant that will be true if
        the type has the proper member function for the given archive. */
    #ifdef CEREAL_OLDER_GCC
    #define CEREAL_MAKE_HAS_MEMBER_TEST(name)                                                                                      \
    template <class T, class A, class SFINAE = void>                                                                               \
    struct has_member_##name : no {};                                                                                              \
    template <class T, class A>                                                                                                    \
    struct has_member_##name<T, A,                                                                                                 \
      typename detail::Void< decltype( cereal::access::member_##name( std::declval<A&>(), std::declval<T&>() ) ) >::type> : yes {}
    #else // NOT CEREAL_OLDER_GCC
    #define CEREAL_MAKE_HAS_MEMBER_TEST(name)                                                                                      \
    namespace detail                                                                                                               \
    {                                                                                                                              \
      template <class T, class A>                                                                                                  \
      struct has_member_##name##_impl                                                                                              \
      {                                                                                                                            \
        template <class TT, class AA>                                                                                              \
        static auto test(int) -> decltype( cereal::access::member_##name( std::declval<AA&>(), std::declval<TT&>() ), yes());      \
        template <class, class>                                                                                                    \
        static no test(...);                                                                                                       \
        static const bool value = std::is_same<decltype(test<T, A>(0)), yes>::value;                                               \
      };                                                                                                                           \
    } /* end namespace detail */                                                                                                   \
    template <class T, class A>                                                                                                    \
    struct has_member_##name : std::integral_constant<bool, detail::has_member_##name##_impl<T, A>::value> {}
    #endif // NOT CEREAL_OLDER_GCC

    //! Creates a test for whether a non const non-member function exists
    /*! This creates a class derived from std::integral_constant that will be true if
        the type has the proper non-member function for the given archive. */
    #define CEREAL_MAKE_HAS_NON_MEMBER_TEST(name)                                                                                  \
    namespace detail                                                                                                               \
    {                                                                                                                              \
      template <class T, class A>                                                                                                  \
      struct has_non_member_##name##_impl                                                                                          \
      {                                                                                                                            \
        template <class TT, class AA>                                                                                              \
        static auto test(int) -> decltype( name( std::declval<AA&>(), std::declval<TT&>() ), yes());                               \
        template <class, class>                                                                                                    \
        static no test( ... );                                                                                                     \
        static const bool value = std::is_same<decltype( test<T, A>( 0 ) ), yes>::value;                                           \
      };                                                                                                                           \
    } /* end namespace detail */                                                                                                   \
    template <class T, class A>                                                                                                    \
    struct has_non_member_##name : std::integral_constant<bool, detail::has_non_member_##name##_impl<T, A>::value> {}

    //! Creates a test for whether a non const member function exists with a version parameter
    /*! This creates a class derived from std::integral_constant that will be true if
        the type has the proper member function for the given archive. */
    #ifdef CEREAL_OLDER_GCC
    #define CEREAL_MAKE_HAS_MEMBER_VERSIONED_TEST(name)                                                                            \
    template <class T, class A, class SFINAE = void>                                                                               \
    struct has_member_versioned_##name : no {};                                                                                    \
    template <class T, class A>                                                                                                    \
    struct has_member_versioned_##name<T, A,                                                                                       \
      typename detail::Void< decltype( cereal::access::member_##name( std::declval<A&>(), std::declval<T&>(), 0 ) ) >::type> : yes {}
    #else // NOT CEREAL_OLDER_GCC
    #define CEREAL_MAKE_HAS_MEMBER_VERSIONED_TEST(name)                                                                            \
    namespace detail                                                                                                               \
    {                                                                                                                              \
      template <class T, class A>                                                                                                  \
      struct has_member_versioned_##name##_impl                                                                                    \
      {                                                                                                                            \
        template <class TT, class AA>                                                                                              \
        static auto test(int) -> decltype( cereal::access::member_##name( std::declval<AA&>(), std::declval<TT&>(), 0 ), yes());   \
        template <class, class>                                                                                                    \
        static no test(...);                                                                                                       \
        static const bool value = std::is_same<decltype(test<T, A>(0)), yes>::value;                                               \
      };                                                                                                                           \
    } /* end namespace detail */                                                                                                   \
    template <class T, class A>                                                                                                    \
    struct has_member_versioned_##name : std::integral_constant<bool, detail::has_member_versioned_##name##_impl<T, A>::value> {}
    #endif // NOT CEREAL_OLDER_GCC

    //! Creates a test for whether a non const non-member function exists with a version parameter
    /*! This creates a class derived from std::integral_constant that will be true if
        the type has the proper non-member function for the given archive. */
    #define CEREAL_MAKE_HAS_NON_MEMBER_VERSIONED_TEST(name)                                                                        \
    namespace detail                                                                                                               \
    {                                                                                                                              \
      template <class T, class A>                                                                                                  \
      struct has_non_member_versioned_##name##_impl                                                                                \
      {                                                                                                                            \
        template <class TT, class AA>                                                                                              \
        static auto test(int) -> decltype( name( std::declval<AA&>(), std::declval<TT&>(), 0 ), yes());                            \
        template <class, class>                                                                                                    \
        static no test( ... );                                                                                                     \
        static const bool value = std::is_same<decltype( test<T, A>( 0 ) ), yes>::value;                                           \
      };                                                                                                                           \
    } /* end namespace detail */                                                                                                   \
    template <class T, class A>                                                                                                    \
    struct has_non_member_versioned_##name : std::integral_constant<bool, detail::has_non_member_versioned_##name##_impl<T, A>::value> {}

    // ######################################################################
    // Member load_and_construct
    template<typename T, typename A>
    struct has_member_load_and_construct :
      std::integral_constant<bool, std::is_same<decltype( access::load_and_construct<T>( std::declval<A&>(), std::declval< ::cereal::construct<T>&>() ) ), void>::value> {};

    // ######################################################################
    // Non Member load_and_construct
    template<typename T, typename A>
    struct has_non_member_load_and_construct : std::integral_constant<bool,
      std::is_same<decltype( LoadAndConstruct<T>::load_and_construct( std::declval<A&>(), std::declval< ::cereal::construct<T>&>() ) ), void>::value> {};

    // ######################################################################
    // Has either a member or non member allocate
    template<typename T, typename A>
    struct has_load_and_construct : std::integral_constant<bool,
      has_member_load_and_construct<T, A>::value || has_non_member_load_and_construct<T, A>::value>
    { };

    // ######################################################################
    // Member Serialize
    CEREAL_MAKE_HAS_MEMBER_TEST(serialize);

    // ######################################################################
    // Member Serialize (versioned)
    CEREAL_MAKE_HAS_MEMBER_VERSIONED_TEST(serialize);

    // ######################################################################
    // Non Member Serialize
    CEREAL_MAKE_HAS_NON_MEMBER_TEST(serialize);

    // ######################################################################
    // Non Member Serialize (versioned)
    CEREAL_MAKE_HAS_NON_MEMBER_VERSIONED_TEST(serialize);

    // ######################################################################
    // Member Load
    CEREAL_MAKE_HAS_MEMBER_TEST(load);

    // ######################################################################
    // Member Load (versioned)
    CEREAL_MAKE_HAS_MEMBER_VERSIONED_TEST(load);

    // ######################################################################
    // Non Member Load
    CEREAL_MAKE_HAS_NON_MEMBER_TEST(load);

    // ######################################################################
    // Non Member Load (versioned)
    CEREAL_MAKE_HAS_NON_MEMBER_VERSIONED_TEST(load);

    // ######################################################################
    // Member Save
    namespace detail
    {
      template <class T, class A>
      struct has_member_save_impl
      {
        #ifdef CEREAL_OLDER_GCC
        template <class TT, class AA, class SFINAE = void>
        struct test : no {};
        template <class TT, class AA>
        struct test<TT, AA,
          typename detail::Void< decltype( cereal::access::member_save( std::declval<AA&>(), std::declval<TT const &>() ) ) >::type> : yes {};
        static const bool value = test<T, A>();

        template <class TT, class AA, class SFINAE = void>
        struct test2 : no {};
        template <class TT, class AA>
        struct test2<TT, AA,
          typename detail::Void< decltype( cereal::access::member_save_non_const( std::declval<AA&>(), std::declval<typename std::remove_const<TT>::type&>() ) ) >::type> : yes {};
        static const bool not_const_type = test2<T, A>();
        #else // NOT CEREAL_OLDER_GCC =========================================
        template <class TT, class AA>
        static auto test(int) -> decltype( cereal::access::member_save( std::declval<AA&>(), std::declval<TT const &>() ), yes());
        template <class, class>
        static no test(...);
        static const bool value = std::is_same<decltype(test<T, A>(0)), yes>::value;

        template <class TT, class AA>
        static auto test2(int) -> decltype( cereal::access::member_save_non_const( std::declval<AA &>(), std::declval<typename std::remove_const<TT>::type&>() ), yes());
        template <class, class>
        static no test2(...);
        static const bool not_const_type = std::is_same<decltype(test2<T, A>(0)), yes>::value;
        #endif // NOT CEREAL_OLDER_GCC
      };
    } // end namespace detail

    template <class T, class A>
    struct has_member_save : std::integral_constant<bool, detail::has_member_save_impl<T, A>::value>
    {
      typedef typename detail::has_member_save_impl<T, A> check;
      static_assert( check::value || !check::not_const_type,
        "cereal detected a non-const save. \n "
        "save member functions must always be const" );
    };

    // ######################################################################
    // Member Save (versioned)
    namespace detail
    {
      template <class T, class A>
      struct has_member_versioned_save_impl
      {
        #ifdef CEREAL_OLDER_GCC
        template <class TT, class AA, class SFINAE = void>
        struct test : no {};
        template <class TT, class AA>
        struct test<TT, AA,
          typename detail::Void< decltype( cereal::access::member_save( std::declval<AA&>(), std::declval<TT const &>(), 0 ) ) >::type> : yes {};
        static const bool value = test<T, A>();

        template <class TT, class AA, class SFINAE = void>
        struct test2 : no {};
        template <class TT, class AA>
        struct test2<TT, AA,
          typename detail::Void< decltype( cereal::access::member_save_non_const( std::declval<AA&>(), std::declval<typename std::remove_const<TT>::type&>(), 0 ) ) >::type> : yes {};
        static const bool not_const_type = test2<T, A>();
        #else // NOT CEREAL_OLDER_GCC =========================================
        template <class TT, class AA>
        static auto test(int) -> decltype( cereal::access::member_save( std::declval<AA&>(), std::declval<TT const &>(), 0 ), yes());
        template <class, class>
        static no test(...);
        static const bool value = std::is_same<decltype(test<T, A>(0)), yes>::value;

        template <class TT, class AA>
        static auto test2(int) -> decltype( cereal::access::member_save_non_const( std::declval<AA &>(), std::declval<typename std::remove_const<TT>::type&>(), 0 ), yes());
        template <class, class>
        static no test2(...);
        static const bool not_const_type = std::is_same<decltype(test2<T, A>(0)), yes>::value;
        #endif // NOT_CEREAL_OLDER_GCC
      };
    } // end namespace detail

    template <class T, class A>
    struct has_member_versioned_save : std::integral_constant<bool, detail::has_member_versioned_save_impl<T, A>::value>
    {
      typedef typename detail::has_member_versioned_save_impl<T, A> check;
      static_assert( check::value || !check::not_const_type,
        "cereal detected a versioned non-const save. \n "
        "save member functions must always be const" );
    };

    // ######################################################################
    // Non-Member Save
    namespace detail
    {
      template <class T, class A>
      struct has_non_member_save_impl
      {
        template <class TT, class AA>
        static auto test(int) -> decltype( save( std::declval<AA&>(), std::declval<TT const &>() ), yes());
        template <class, class>
        static no test(...);
        static const bool value = std::is_same<decltype(test<T, A>(0)), yes>::value;

        template <class TT, class AA>
        static auto test2(int) -> decltype( save( std::declval<AA &>(), std::declval<typename std::remove_const<TT>::type&>() ), yes());
        template <class, class>
        static no test2(...);
        static const bool not_const_type = std::is_same<decltype(test2<T, A>(0)), yes>::value;
      };
    } // end namespace detail

    template <class T, class A>
    struct has_non_member_save : std::integral_constant<bool, detail::has_non_member_save_impl<T, A>::value>
    {
      typedef typename detail::has_non_member_save_impl<T, A> check;
      static_assert( check::value || !check::not_const_type,
        "cereal detected a non-const type parameter in non-member save. \n "
        "save non-member functions must always pass their types as const" );
    };

    // ######################################################################
    // Non-Member Save (versioned)
    namespace detail
    {
      template <class T, class A>
      struct has_non_member_versioned_save_impl
      {
        template <class TT, class AA>
        static auto test(int) -> decltype( save( std::declval<AA&>(), std::declval<TT const &>(), 0 ), yes());
        template <class, class>
        static no test(...);
        static const bool value = std::is_same<decltype(test<T, A>(0)), yes>::value;

        template <class TT, class AA>
        static auto test2(int) -> decltype( save( std::declval<AA &>(), std::declval<typename std::remove_const<TT>::type&>(), 0 ), yes());
        template <class, class>
        static no test2(...);
        static const bool not_const_type = std::is_same<decltype(test2<T, A>(0)), yes>::value;
      };
    } // end namespace detail

    template <class T, class A>
    struct has_non_member_versioned_save : std::integral_constant<bool, detail::has_non_member_versioned_save_impl<T, A>::value>
    {
      typedef typename detail::has_non_member_versioned_save_impl<T, A> check;
      static_assert( check::value || !check::not_const_type,
        "cereal detected a non-const type parameter in versioned non-member save. \n "
        "save non-member functions must always pass their types as const" );
    };

    // ######################################################################
    // Minimal Utilities
    namespace detail
    {
      // Determines if the provided type is an std::string
      template <class> struct is_string : std::false_type {};

      template <class CharT, class Traits, class Alloc>
      struct is_string<std::basic_string<CharT, Traits, Alloc>> : std::true_type {};
    }

    // Determines if the type is valid for use with a minimal serialize function
    template <class T>
    struct is_minimal_type : std::integral_constant<bool, detail::is_string<T>::value ||
      std::is_arithmetic<T>::value> {};

    // ######################################################################
    // Member Save Minimal
    namespace detail
    {
      template <class T, class A>
      struct has_member_save_minimal_impl
      {
        #ifdef CEREAL_OLDER_GCC
        template <class TT, class AA, class SFINAE = void>
        struct test : no {};
        template <class TT, class AA>
        struct test<TT, AA,
          typename detail::Void< decltype( cereal::access::member_save_minimal( std::declval<AA const &>(), std::declval<TT const &>() ) ) >::type> : yes {};
        static const bool value = test<T, A>();

        template <class TT, class AA, class SFINAE = void>
        struct test2 : no {};
        template <class TT, class AA>
        struct test2<TT, AA,
          typename detail::Void< decltype( cereal::access::member_save_minimal_non_const( std::declval<AA const &>(), std::declval<typename std::remove_const<TT>::type&>() ) ) >::type> : yes {};
        static const bool not_const_type = test2<T, A>();
        #else // NOT CEREAL_OLDER_GCC =========================================
        template <class TT, class AA>
        static auto test(int) -> decltype( cereal::access::member_save_minimal( std::declval<AA const &>(), std::declval<TT const &>() ), yes());
        template <class, class>
        static no test(...);
        static const bool value = std::is_same<decltype(test<T, A>(0)), yes>::value;

        template <class TT, class AA>
        static auto test2(int) -> decltype( cereal::access::member_save_minimal_non_const( std::declval<AA const &>(), std::declval<typename std::remove_const<TT>::type&>() ), yes());
        template <class, class>
        static no test2(...);
        static const bool not_const_type = std::is_same<decltype(test2<T, A>(0)), yes>::value;
        #endif // NOT CEREAL_OLDER_GCC

        static const bool valid = value || !not_const_type;
      };

      template <class T, class A, bool Valid>
      struct get_member_save_minimal_type { using type = void; };

      template <class T, class A>
      struct get_member_save_minimal_type<T, A, true>
      {
        using type = decltype( cereal::access::member_save_minimal( std::declval<A const &>(), std::declval<T const &>() ) );
      };
    } // end namespace detail

    template <class T, class A>
    struct has_member_save_minimal : std::integral_constant<bool, detail::has_member_save_minimal_impl<T, A>::value>
    {
      typedef typename detail::has_member_save_minimal_impl<T, A> check;
      static_assert( check::valid,
        "cereal detected a non-const member save_minimal.  "
        "save_minimal member functions must always be const" );

      using type = typename detail::get_member_save_minimal_type<T, A, check::value>::type;
      static_assert( (check::value && is_minimal_type<type>::value) || !check::value,
        "cereal detected a member save_minimal with an invalid return type.  "
        "return type must be arithmetic or string" );
    };

    // ######################################################################
    // Member Save Minimal (versioned)
    namespace detail
    {
      template <class T, class A>
      struct has_member_versioned_save_minimal_impl
      {
        #ifdef CEREAL_OLDER_GCC
        template <class TT, class AA, class SFINAE = void>
        struct test : no {};
        template <class TT, class AA>
        struct test<TT, AA,
          typename detail::Void< decltype( cereal::access::member_save_minimal( std::declval<AA const &>(), std::declval<TT const &>(), 0 ) ) >::type> : yes {};
        static const bool value = test<T, A>();

        template <class TT, class AA, class SFINAE = void>
        struct test2 : no {};
        template <class TT, class AA>
        struct test2<TT, AA,
          typename detail::Void< decltype( cereal::access::member_save_minimal_non_const( std::declval<AA const &>(), std::declval<typename std::remove_const<TT>::type&>(), 0 ) ) >::type> : yes {};
        static const bool not_const_type = test2<T, A>();
        #else // NOT CEREAL_OLDER_GCC =========================================
        template <class TT, class AA>
        static auto test(int) -> decltype( cereal::access::member_save_minimal( std::declval<AA const &>(), std::declval<TT const &>(), 0 ), yes());
        template <class, class>
        static no test(...);
        static const bool value = std::is_same<decltype(test<T, A>(0)), yes>::value;

        template <class TT, class AA>
        static auto test2(int) -> decltype( cereal::access::member_save_minimal_non_const( std::declval<AA const &>(), std::declval<typename std::remove_const<TT>::type&>(), 0 ), yes());
        template <class, class>
        static no test2(...);
        static const bool not_const_type = std::is_same<decltype(test2<T, A>(0)), yes>::value;
        #endif // NOT_CEREAL_OLDER_GCC

        static const bool valid = value || !not_const_type;
      };

      template <class T, class A, bool Valid>
      struct get_member_versioned_save_minimal_type { using type = void; };

      template <class T, class A>
      struct get_member_versioned_save_minimal_type<T, A, true>
      {
        using type = decltype( cereal::access::member_save_minimal( std::declval<A const &>(), std::declval<T const &>(), 0 ) );
      };
    } // end namespace detail

    template <class T, class A>
    struct has_member_versioned_save_minimal : std::integral_constant<bool, detail::has_member_versioned_save_minimal_impl<T, A>::value>
    {
      typedef typename detail::has_member_versioned_save_minimal_impl<T, A> check;
      static_assert( check::valid,
        "cereal detected a versioned non-const member save_minimal.  "
        "save_minimal member functions must always be const" );

      using type = typename detail::get_member_versioned_save_minimal_type<T, A, check::value>::type;
      static_assert( (check::value && is_minimal_type<type>::value) || !check::value,
        "cereal detected a versioned member save_minimal with an invalid return type.  "
        "return type must be arithmetic or string" );
    };

    // ######################################################################
    // Non-Member Save Minimal
    namespace detail
    {
      template <class T, class A>
      struct has_non_member_save_minimal_impl
      {
        template <class TT, class AA>
        static auto test(int) -> decltype( save_minimal( std::declval<AA const &>(), std::declval<TT const &>() ), yes());
        template <class, class>
        static no test(...);
        static const bool value = std::is_same<decltype(test<T, A>(0)), yes>::value;

        template <class TT, class AA>
        static auto test2(int) -> decltype( save_minimal( std::declval<AA const &>(), std::declval<typename std::remove_const<TT>::type&>() ), yes());
        template <class, class>
        static no test2(...);
        static const bool not_const_type = std::is_same<decltype(test2<T, A>(0)), yes>::value;

        static const bool valid = value || !not_const_type;
      };

      template <class T, class A, bool Valid>
      struct get_non_member_save_minimal_type { using type = void; };

      template <class T, class A>
      struct get_non_member_save_minimal_type <T, A, true>
      {
        using type = decltype( save_minimal( std::declval<A const &>(), std::declval<T const &>() ) );
      };
    } // end namespace detail

    template <class T, class A>
    struct has_non_member_save_minimal : std::integral_constant<bool, detail::has_non_member_save_minimal_impl<T, A>::value>
    {
      typedef typename detail::has_non_member_save_minimal_impl<T, A> check;
      static_assert( check::valid,
        "cereal detected a non-const type parameter in non-member save_minimal.  "
        "save_minimal non-member functions must always pass their types as const" );

      using type = typename detail::get_non_member_save_minimal_type<T, A, check::value>::type;
      static_assert( (check::value && is_minimal_type<type>::value) || !check::value,
        "cereal detected a non-member save_minimal with an invalid return type.  "
        "return type must be arithmetic or string" );
    };

    // ######################################################################
    // Non-Member Save Minimal (versioned)
    namespace detail
    {
      template <class T, class A>
      struct has_non_member_versioned_save_minimal_impl
      {
        template <class TT, class AA>
        static auto test(int) -> decltype( save_minimal( std::declval<AA const &>(), std::declval<TT const &>(), 0 ), yes());
        template <class, class>
        static no test(...);
        static const bool value = std::is_same<decltype(test<T, A>(0)), yes>::value;

        template <class TT, class AA>
        static auto test2(int) -> decltype( save_minimal( std::declval<AA const &>(), std::declval<typename std::remove_const<TT>::type&>(), 0 ), yes());
        template <class, class>
        static no test2(...);
        static const bool not_const_type = std::is_same<decltype(test2<T, A>(0)), yes>::value;

        static const bool valid = value || !not_const_type;
      };

      template <class T, class A, bool Valid>
      struct get_non_member_versioned_save_minimal_type { using type = void; };

      template <class T, class A>
      struct get_non_member_versioned_save_minimal_type <T, A, true>
      {
        using type = decltype( save_minimal( std::declval<A const &>(), std::declval<T const &>(), 0 ) );
      };
    } // end namespace detail

    template <class T, class A>
    struct has_non_member_versioned_save_minimal : std::integral_constant<bool, detail::has_non_member_versioned_save_minimal_impl<T, A>::value>
    {
      typedef typename detail::has_non_member_versioned_save_minimal_impl<T, A> check;
      static_assert( check::valid,
        "cereal detected a non-const type parameter in versioned non-member save_minimal.  "
        "save_minimal non-member functions must always pass their types as const" );

      using type = typename detail::get_non_member_versioned_save_minimal_type<T, A, check::value>::type;
      static_assert( (check::value && is_minimal_type<type>::value) || !check::value,
        "cereal detected a non-member versioned save_minimal with an invalid return type.  "
        "return type must be arithmetic or string" );
    };

    // ######################################################################
    // Member Load Minimal
    namespace detail
    {
      //! Used to help strip away conversion wrappers
      /*! If someone writes a non-member load/save minimal function that accepts its
          parameter as some generic template type and needs to perform trait checks
          on that type, our NoConvert wrappers will interfere with this.  Using
          the struct strip_minmal, users can strip away our wrappers to get to
          the underlying type, allowing traits to work properly */
      struct NoConvertBase {};

      //! A struct that prevents implicit conversion
      /*! Any type instantiated with this struct will be unable to implicitly convert
          to another type.  Is designed to only allow conversion to Source const &.

          @tparam Source the type of the original source */
      template <class Source>
      struct NoConvertConstRef : NoConvertBase
      {
        using type = Source; //!< Used to get underlying type easily

        template <class Dest, class = typename std::enable_if<std::is_same<Source, Dest>::value>::type>
        operator Dest () = delete;

        //! only allow conversion if the types are the same and we are converting into a const reference
        template <class Dest, class = typename std::enable_if<std::is_same<Source, Dest>::value>::type>
        operator Dest const & ();
      };

      //! A struct that prevents implicit conversion
      /*! Any type instantiated with this struct will be unable to implicitly convert
          to another type.  Is designed to only allow conversion to Source &.

          @tparam Source the type of the original source */
      template <class Source>
      struct NoConvertRef : NoConvertBase
      {
        using type = Source; //!< Used to get underlying type easily

        template <class Dest, class = typename std::enable_if<std::is_same<Source, Dest>::value>::type>
        operator Dest () = delete;

        #ifdef __clang__
        template <class Dest, class = typename std::enable_if<std::is_same<Source, Dest>::value>::type>
        operator Dest const & () = delete;
        #endif // __clang__

        //! only allow conversion if the types are the same and we are converting into a const reference
        template <class Dest, class = typename std::enable_if<std::is_same<Source, Dest>::value>::type>
        operator Dest & ();
      };

      //! A type that can implicitly convert to anything else
      struct AnyConvert
      {
        template <class Dest>
        operator Dest & ();

        template <class Dest>
        operator Dest const & () const;
      };

      // Our strategy here is to first check if a function matching the signature more or less exists
      // (allow anything like load_minimal(xxx) using AnyConvert, and then secondly enforce
      // that it has the correct signature using NoConvertConstRef
      #ifdef CEREAL_OLDER_GCC
      template <class T, class A, class SFINAE = void>
      struct has_member_load_minimal_impl : no {};
      template <class T, class A>
      struct has_member_load_minimal_impl<T, A,
        typename detail::Void<decltype( cereal::access::member_load_minimal( std::declval<A const &>(), std::declval<T&>(), AnyConvert() ) ) >::type> : yes {};

      template <class T, class A, class U, class SFINAE = void>
      struct has_member_load_minimal_type_impl : no {};
      template <class T, class A, class U>
      struct has_member_load_minimal_type_impl<T, A, U,
        typename detail::Void<decltype( cereal::access::member_load_minimal( std::declval<A const &>(), std::declval<T&>(), NoConvertConstRef<U>() ) ) >::type> : yes {};
      #else // NOT CEREAL_OLDER_GCC
      template <class T, class A>
      struct has_member_load_minimal_impl
      {
        template <class TT, class AA>
        static auto test(int) -> decltype( cereal::access::member_load_minimal( std::declval<AA const &>(), std::declval<TT&>(), AnyConvert() ), yes());
        template <class, class>
        static no test(...);
        static const bool value = std::is_same<decltype(test<T, A>(0)), yes>::value;
      };

      template <class T, class A, class U>
      struct has_member_load_minimal_type_impl
      {
        template <class TT, class AA, class UU>
        static auto test(int) -> decltype( cereal::access::member_load_minimal( std::declval<AA const &>(), std::declval<TT&>(), NoConvertConstRef<U>() ), yes());
        template <class, class, class>
        static no test(...);
        static const bool value = std::is_same<decltype(test<T, A, U>(0)), yes>::value;
      };
      #endif // NOT CEREAL_OLDER_GCC

      template <class T, class A, bool Valid>
      struct has_member_load_minimal_wrapper : std::false_type {};

      template <class T, class A>
      struct has_member_load_minimal_wrapper<T, A, true>
      {
        static_assert( has_member_save_minimal<T, A>::value,
          "cereal detected member load_minimal but no valid member save_minimal.  "
          "cannot evaluate correctness of load_minimal without valid save_minimal." );

        using SaveType = typename detail::get_member_save_minimal_type<T, A, true>::type;
        const static bool value = has_member_load_minimal_impl<T, A>::value;
        const static bool valid = has_member_load_minimal_type_impl<T, A, SaveType>::value;

        static_assert( valid || !value, "cereal detected different or invalid types in corresponding member load_minimal and save_minimal functions.  "
            "the paramater to load_minimal must be a constant reference to the type that save_minimal returns." );
      };
    }

    template <class T, class A>
    struct has_member_load_minimal : std::integral_constant<bool,
      detail::has_member_load_minimal_wrapper<T, A, detail::has_member_load_minimal_impl<T, A>::value>::value> {};

    // ######################################################################
    // Member Load Minimal (versioned)
    namespace detail
    {
      #ifdef CEREAL_OLDER_GCC
      template <class T, class A, class SFINAE = void>
      struct has_member_versioned_load_minimal_impl : no {};
      template <class T, class A>
      struct has_member_versioned_load_minimal_impl<T, A,
        typename detail::Void<decltype( cereal::access::member_load_minimal( std::declval<A const &>(), std::declval<T&>(), AnyConvert(), 0 ) ) >::type> : yes {};

      template <class T, class A, class U, class SFINAE = void>
      struct has_member_versioned_load_minimal_type_impl : no {};
      template <class T, class A, class U>
      struct has_member_versioned_load_minimal_type_impl<T, A, U,
        typename detail::Void<decltype( cereal::access::member_load_minimal( std::declval<A const &>(), std::declval<T&>(), NoConvertConstRef<U>(), 0 ) ) >::type> : yes {};
      #else // NOT CEREAL_OLDER_GCC
      template <class T, class A>
      struct has_member_versioned_load_minimal_impl
      {
        template <class TT, class AA>
        static auto test(int) -> decltype( cereal::access::member_load_minimal( std::declval<AA const &>(), std::declval<TT&>(), AnyConvert(), 0 ), yes());
        template <class, class>
        static no test(...);
        static const bool value = std::is_same<decltype(test<T, A>(0)), yes>::value;
      };

      template <class T, class A, class U>
      struct has_member_versioned_load_minimal_type_impl
      {
        template <class TT, class AA, class UU>
        static auto test(int) -> decltype( cereal::access::member_load_minimal( std::declval<AA const &>(), std::declval<TT&>(), NoConvertConstRef<U>(), 0 ), yes());
        template <class, class, class>
        static no test(...);
        static const bool value = std::is_same<decltype(test<T, A, U>(0)), yes>::value;
      };
      #endif // NOT CEREAL_OLDER_GCC

      template <class T, class A, bool Valid>
      struct has_member_versioned_load_minimal_wrapper : std::false_type {};

      template <class T, class A>
      struct has_member_versioned_load_minimal_wrapper<T, A, true>
      {
        static_assert( has_member_versioned_save_minimal<T, A>::value,
          "cereal detected member versioned load_minimal but no valid member versioned save_minimal.  "
          "cannot evaluate correctness of load_minimal without valid save_minimal." );

        using SaveType = typename detail::get_member_versioned_save_minimal_type<T, A, true>::type;
        const static bool value = has_member_versioned_load_minimal_impl<T, A>::value;
        const static bool valid = has_member_versioned_load_minimal_type_impl<T, A, SaveType>::value;

        static_assert( valid || !value, "cereal detected different or invalid types in corresponding member versioned load_minimal and save_minimal functions.  "
            "the paramater to load_minimal must be a constant reference to the type that save_minimal returns." );
      };
    }

    template <class T, class A>
    struct has_member_versioned_load_minimal : std::integral_constant<bool,
      detail::has_member_versioned_load_minimal_wrapper<T, A, detail::has_member_versioned_load_minimal_impl<T, A>::value>::value> {};

    // ######################################################################
    // Non-Member Load Minimal
    namespace detail
    {
      #ifdef CEREAL_OLDER_GCC
      void load_minimal(); // prevents nonsense complaining about not finding this
      void save_minimal();
      #endif // CEREAL_OLDER_GCC

      // See notes from member load_minimal
      template <class T, class A, class U = void>
      struct has_non_member_load_minimal_impl
      {
        template <class TT, class AA>
        static auto test(int) -> decltype( load_minimal( std::declval<AA const &>(), std::declval<TT&>(), AnyConvert() ), yes() );
        template <class, class>
        static no test( ... );
        static const bool exists = std::is_same<decltype( test<T, A>( 0 ) ), yes>::value;

        template <class TT, class AA, class UU>
        static auto test2(int) -> decltype( load_minimal( std::declval<AA const &>(), std::declval<TT&>(), NoConvertConstRef<UU>() ), yes() );
        template <class, class, class>
        static no test2( ... );
        static const bool valid = std::is_same<decltype( test2<T, A, U>( 0 ) ), yes>::value;

        template <class TT, class AA>
        static auto test3(int) -> decltype( load_minimal( std::declval<AA const &>(), NoConvertRef<TT>(), AnyConvert() ), yes() );
        template <class, class>
        static no test3( ... );
        static const bool const_valid = std::is_same<decltype( test3<T, A>( 0 ) ), yes>::value;
      };

      template <class T, class A, bool Valid>
      struct has_non_member_load_minimal_wrapper : std::false_type {};

      template <class T, class A>
      struct has_non_member_load_minimal_wrapper<T, A, true>
      {
        static_assert( detail::has_non_member_save_minimal_impl<T, A>::valid,
          "cereal detected non-member load_minimal but no valid non-member save_minimal.  "
          "cannot evaluate correctness of load_minimal without valid save_minimal." );

        using SaveType = typename detail::get_non_member_save_minimal_type<T, A, true>::type;
        using check = has_non_member_load_minimal_impl<T, A, SaveType>;
        static const bool value = check::exists;

        static_assert( check::valid || !check::exists, "cereal detected different types in corresponding non-member load_minimal and save_minimal functions.  "
            "the paramater to load_minimal must be a constant reference to the type that save_minimal returns." );
        static_assert( check::const_valid || !check::exists, "cereal detected an invalid serialization type parameter in non-member load_minimal.  "
            "load_minimal non-member functions must accept their serialization type by non-const reference" );
      };
    }

    template <class T, class A>
    struct has_non_member_load_minimal : std::integral_constant<bool,
      detail::has_non_member_load_minimal_wrapper<T, A, detail::has_non_member_load_minimal_impl<T, A>::exists>::value> {};

    // ######################################################################
    // Non-Member Load Minimal (versioned)
    namespace detail
    {
      // See notes from member load_minimal
      template <class T, class A, class U = void>
      struct has_non_member_versioned_load_minimal_impl
      {
        template <class TT, class AA>
        static auto test(int) -> decltype( load_minimal( std::declval<AA const &>(), std::declval<TT&>(), AnyConvert(), 0 ), yes() );
        template <class, class>
        static no test( ... );
        static const bool exists = std::is_same<decltype( test<T, A>( 0 ) ), yes>::value;

        template <class TT, class AA, class UU>
        static auto test2(int) -> decltype( load_minimal( std::declval<AA const &>(), std::declval<TT&>(), NoConvertConstRef<UU>(), 0 ), yes() );
        template <class, class, class>
        static no test2( ... );
        static const bool valid = std::is_same<decltype( test2<T, A, U>( 0 ) ), yes>::value;

        template <class TT, class AA>
        static auto test3(int) -> decltype( load_minimal( std::declval<AA const &>(), NoConvertRef<TT>(), AnyConvert(), 0 ), yes() );
        template <class, class>
        static no test3( ... );
        static const bool const_valid = std::is_same<decltype( test3<T, A>( 0 ) ), yes>::value;
      };

      template <class T, class A, bool Valid>
      struct has_non_member_versioned_load_minimal_wrapper : std::false_type {};

      template <class T, class A>
      struct has_non_member_versioned_load_minimal_wrapper<T, A, true>
      {
        static_assert( detail::has_non_member_versioned_save_minimal_impl<T, A>::valid,
          "cereal detected non-member versioned load_minimal but no valid non-member versioned save_minimal.  "
          "cannot evaluate correctness of load_minimal without valid save_minimal." );

        using SaveType = typename detail::get_non_member_versioned_save_minimal_type<T, A, true>::type;
        using check = has_non_member_versioned_load_minimal_impl<T, A, SaveType>;
        static const bool value = check::exists;

        static_assert( check::valid || !check::exists, "cereal detected different types in corresponding non-member versioned load_minimal and save_minimal functions.  "
            "the paramater to load_minimal must be a constant reference to the type that save_minimal returns." );
        static_assert( check::const_valid || !check::exists, "cereal detected an invalid serialization type parameter in non-member versioned load_minimal.  "
            "load_minimal non-member versioned functions must accept their serialization type by non-const reference" );
      };
    }

    template <class T, class A>
    struct has_non_member_versioned_load_minimal : std::integral_constant<bool,
      detail::has_non_member_versioned_load_minimal_wrapper<T, A, detail::has_non_member_versioned_load_minimal_impl<T, A>::exists>::value> {};

    // ######################################################################
    template <class T, class InputArchive, class OutputArchive>
    struct has_member_split : std::integral_constant<bool,
      (has_member_load<T, InputArchive>::value && has_member_save<T, OutputArchive>::value) ||
      (has_member_versioned_load<T, InputArchive>::value && has_member_versioned_save<T, OutputArchive>::value)> {};

    // ######################################################################
    template <class T, class InputArchive, class OutputArchive>
    struct has_non_member_split : std::integral_constant<bool,
      (has_non_member_load<T, InputArchive>::value && has_non_member_save<T, OutputArchive>::value) ||
      (has_non_member_versioned_load<T, InputArchive>::value && has_non_member_versioned_save<T, OutputArchive>::value)> {};

    // ######################################################################
    template <class T, class OutputArchive>
    struct is_output_serializable : std::integral_constant<bool,
      has_member_save<T, OutputArchive>::value +
      has_non_member_save<T, OutputArchive>::value +
      has_member_serialize<T, OutputArchive>::value +
      has_non_member_serialize<T, OutputArchive>::value +
      has_member_save_minimal<T, OutputArchive>::value +
      has_non_member_save_minimal<T, OutputArchive>::value +
      /*-versioned---------------------------------------------------------*/
      has_member_versioned_save<T, OutputArchive>::value +
      has_non_member_versioned_save<T, OutputArchive>::value +
      has_member_versioned_serialize<T, OutputArchive>::value +
      has_non_member_versioned_serialize<T, OutputArchive>::value +
      has_member_versioned_save_minimal<T, OutputArchive>::value +
      has_non_member_versioned_save_minimal<T, OutputArchive>::value == 1> {};

    // ######################################################################
    template <class T, class InputArchive>
    struct is_input_serializable : std::integral_constant<bool,
      has_member_load<T, InputArchive>::value +
      has_non_member_load<T, InputArchive>::value +
      has_member_serialize<T, InputArchive>::value +
      has_non_member_serialize<T, InputArchive>::value +
      has_member_load_minimal<T, InputArchive>::value +
      has_non_member_load_minimal<T, InputArchive>::value +
      /*-versioned---------------------------------------------------------*/
      has_member_versioned_load<T, InputArchive>::value +
      has_non_member_versioned_load<T, InputArchive>::value +
      has_member_versioned_serialize<T, InputArchive>::value +
      has_non_member_versioned_serialize<T, InputArchive>::value +
      has_member_versioned_load_minimal<T, InputArchive>::value +
      has_non_member_versioned_load_minimal<T, InputArchive>::value == 1> {};

    // ######################################################################
    template <class T, class OutputArchive>
    struct has_invalid_output_versioning : std::integral_constant<bool,
      (has_member_versioned_save<T, OutputArchive>::value && has_member_save<T, OutputArchive>::value) ||
      (has_non_member_versioned_save<T, OutputArchive>::value && has_non_member_save<T, OutputArchive>::value) ||
      (has_member_versioned_serialize<T, OutputArchive>::value && has_member_serialize<T, OutputArchive>::value) ||
      (has_non_member_versioned_serialize<T, OutputArchive>::value && has_non_member_serialize<T, OutputArchive>::value) ||
      (has_member_versioned_save_minimal<T, OutputArchive>::value && has_member_save_minimal<T, OutputArchive>::value) ||
      (has_non_member_versioned_save_minimal<T, OutputArchive>::value &&  has_non_member_save_minimal<T, OutputArchive>::value)> {};

    // ######################################################################
    template <class T, class InputArchive>
    struct has_invalid_input_versioning : std::integral_constant<bool,
      (has_member_versioned_load<T, InputArchive>::value && has_member_load<T, InputArchive>::value) ||
      (has_non_member_versioned_load<T, InputArchive>::value && has_non_member_load<T, InputArchive>::value) ||
      (has_member_versioned_serialize<T, InputArchive>::value && has_member_serialize<T, InputArchive>::value) ||
      (has_non_member_versioned_serialize<T, InputArchive>::value && has_non_member_serialize<T, InputArchive>::value) ||
      (has_member_versioned_load_minimal<T, InputArchive>::value && has_member_load_minimal<T, InputArchive>::value) ||
      (has_non_member_versioned_load_minimal<T, InputArchive>::value &&  has_non_member_load_minimal<T, InputArchive>::value)> {};

    // ######################################################################
    namespace detail
    {
      template <class T, class A>
      struct is_specialized_member_serialize : std::integral_constant<bool,
        !std::is_base_of<std::false_type, specialize<A, T, specialization::member_serialize>>::value> {};

      template <class T, class A>
      struct is_specialized_member_load_save : std::integral_constant<bool,
        !std::is_base_of<std::false_type, specialize<A, T, specialization::member_load_save>>::value> {};

      template <class T, class A>
      struct is_specialized_member_load_save_minimal : std::integral_constant<bool,
        !std::is_base_of<std::false_type, specialize<A, T, specialization::member_load_save_minimal>>::value> {};

      template <class T, class A>
      struct is_specialized_non_member_serialize : std::integral_constant<bool,
        !std::is_base_of<std::false_type, specialize<A, T, specialization::non_member_serialize>>::value> {};

      template <class T, class A>
      struct is_specialized_non_member_load_save : std::integral_constant<bool,
        !std::is_base_of<std::false_type, specialize<A, T, specialization::non_member_load_save>>::value> {};

      template <class T, class A>
      struct is_specialized_non_member_load_save_minimal : std::integral_constant<bool,
        !std::is_base_of<std::false_type, specialize<A, T, specialization::non_member_load_save_minimal>>::value> {};

      // Considered an error if specialization exists for more than one type
      template <class T, class A>
      struct is_specialized_error : std::integral_constant<bool,
        (is_specialized_member_serialize<T, A>::value +
         is_specialized_member_load_save<T, A>::value +
         is_specialized_member_load_save_minimal<T, A>::value +
         is_specialized_non_member_serialize<T, A>::value +
         is_specialized_non_member_load_save<T, A>::value +
         is_specialized_non_member_load_save_minimal<T, A>::value) <= 1> {};
    } // namespace detail

    template <class T, class A>
    struct is_specialized : std::integral_constant<bool,
      detail::is_specialized_member_serialize<T, A>::value ||
      detail::is_specialized_member_load_save<T, A>::value ||
      detail::is_specialized_member_load_save_minimal<T, A>::value ||
      detail::is_specialized_non_member_serialize<T, A>::value ||
      detail::is_specialized_non_member_load_save<T, A>::value ||
      detail::is_specialized_non_member_load_save_minimal<T, A>::value>
    {
      static_assert(detail::is_specialized_error<T, A>::value, "More than one explicit specialization detected for type.");
    };

    template <class T, class A>
    struct is_specialized_member_serialize : std::integral_constant<bool,
      is_specialized<T, A>::value && detail::is_specialized_member_serialize<T, A>::value>
    {
      static_assert( (is_specialized<T, A>::value && detail::is_specialized_member_serialize<T, A>::value &&
                     (has_member_serialize<T, A>::value || has_member_versioned_serialize<T, A>::value))
                     || !(is_specialized<T, A>::value && detail::is_specialized_member_serialize<T, A>::value),
                     "cereal detected member serialization specialization but no member serialize function" );
    };

    template <class T, class A>
    struct is_specialized_member_load : std::integral_constant<bool,
      is_specialized<T, A>::value && detail::is_specialized_member_load_save<T, A>::value>
    {
      static_assert( (is_specialized<T, A>::value && detail::is_specialized_member_load_save<T, A>::value &&
                     (has_member_load<T, A>::value || has_member_versioned_load<T, A>::value))
                     || !(is_specialized<T, A>::value && detail::is_specialized_member_load_save<T, A>::value),
                     "cereal detected member load specialization but no member load function" );
    };

    template <class T, class A>
    struct is_specialized_member_save : std::integral_constant<bool,
      is_specialized<T, A>::value && detail::is_specialized_member_load_save<T, A>::value>
    {
      static_assert( (is_specialized<T, A>::value && detail::is_specialized_member_load_save<T, A>::value &&
                     (has_member_save<T, A>::value || has_member_versioned_save<T, A>::value))
                     || !(is_specialized<T, A>::value && detail::is_specialized_member_load_save<T, A>::value),
                     "cereal detected member save specialization but no member save function" );
    };

    template <class T, class A>
    struct is_specialized_member_load_minimal : std::integral_constant<bool,
      is_specialized<T, A>::value && detail::is_specialized_member_load_save_minimal<T, A>::value>
    {
      static_assert( (is_specialized<T, A>::value && detail::is_specialized_member_load_save_minimal<T, A>::value &&
                     (has_member_load_minimal<T, A>::value || has_member_versioned_load_minimal<T, A>::value))
                     || !(is_specialized<T, A>::value && detail::is_specialized_member_load_save_minimal<T, A>::value),
                     "cereal detected member load_minimal specialization but no member load_minimal function" );
    };

    template <class T, class A>
    struct is_specialized_member_save_minimal : std::integral_constant<bool,
      is_specialized<T, A>::value && detail::is_specialized_member_load_save_minimal<T, A>::value>
    {
      static_assert( (is_specialized<T, A>::value && detail::is_specialized_member_load_save_minimal<T, A>::value &&
                     (has_member_save_minimal<T, A>::value || has_member_versioned_save_minimal<T, A>::value))
                     || !(is_specialized<T, A>::value && detail::is_specialized_member_load_save_minimal<T, A>::value),
                     "cereal detected member save_minimal specialization but no member save_minimal function" );
    };

    template <class T, class A>
    struct is_specialized_non_member_serialize : std::integral_constant<bool,
      is_specialized<T, A>::value && detail::is_specialized_non_member_serialize<T, A>::value>
    {
      static_assert( (is_specialized<T, A>::value && detail::is_specialized_non_member_serialize<T, A>::value &&
                     (has_non_member_serialize<T, A>::value || has_non_member_versioned_serialize<T, A>::value))
                     || !(is_specialized<T, A>::value && detail::is_specialized_non_member_serialize<T, A>::value),
                     "cereal detected non-member serialization specialization but no non-member serialize function" );
    };

    template <class T, class A>
    struct is_specialized_non_member_load : std::integral_constant<bool,
      is_specialized<T, A>::value && detail::is_specialized_non_member_load_save<T, A>::value>
    {
      static_assert( (is_specialized<T, A>::value && detail::is_specialized_non_member_load_save<T, A>::value &&
                     (has_non_member_load<T, A>::value || has_non_member_versioned_load<T, A>::value))
                     || !(is_specialized<T, A>::value && detail::is_specialized_non_member_load_save<T, A>::value),
                     "cereal detected non-member load specialization but no non-member load function" );
    };

    template <class T, class A>
    struct is_specialized_non_member_save : std::integral_constant<bool,
      is_specialized<T, A>::value && detail::is_specialized_non_member_load_save<T, A>::value>
    {
      static_assert( (is_specialized<T, A>::value && detail::is_specialized_non_member_load_save<T, A>::value &&
                     (has_non_member_save<T, A>::value || has_non_member_versioned_save<T, A>::value))
                     || !(is_specialized<T, A>::value && detail::is_specialized_non_member_load_save<T, A>::value),
                     "cereal detected non-member save specialization but no non-member save function" );
    };

    template <class T, class A>
    struct is_specialized_non_member_load_minimal : std::integral_constant<bool,
      is_specialized<T, A>::value && detail::is_specialized_non_member_load_save_minimal<T, A>::value>
    {
      static_assert( (is_specialized<T, A>::value && detail::is_specialized_non_member_load_save_minimal<T, A>::value &&
                     (has_non_member_load_minimal<T, A>::value || has_non_member_versioned_load_minimal<T, A>::value))
                     || !(is_specialized<T, A>::value && detail::is_specialized_non_member_load_save_minimal<T, A>::value),
                     "cereal detected non-member load specialization but no non-member load function" );
    };

    template <class T, class A>
    struct is_specialized_non_member_save_minimal : std::integral_constant<bool,
      is_specialized<T, A>::value && detail::is_specialized_non_member_load_save_minimal<T, A>::value>
    {
      static_assert( (is_specialized<T, A>::value && detail::is_specialized_non_member_load_save_minimal<T, A>::value &&
                     (has_non_member_save_minimal<T, A>::value || has_non_member_versioned_save_minimal<T, A>::value))
                     || !(is_specialized<T, A>::value && detail::is_specialized_non_member_load_save_minimal<T, A>::value),
                     "cereal detected non-member save specialization but no non-member save function" );
    };

    // ######################################################################
    // detects if a type has any active minimal output serialization
    template <class T, class OutputArchive>
    struct has_minimal_output_serialization : std::integral_constant<bool,
      is_specialized_member_save_minimal<T, OutputArchive>::value ||
      ((has_member_save_minimal<T, OutputArchive>::value ||
        has_non_member_save_minimal<T, OutputArchive>::value ||
        has_member_versioned_save_minimal<T, OutputArchive>::value ||
        has_non_member_versioned_save_minimal<T, OutputArchive>::value) &&
       (!is_specialized_member_serialize<T, OutputArchive>::value ||
        !is_specialized_member_save<T, OutputArchive>::value))> {};

    // ######################################################################
    // detects if a type has any active minimal input serialization
    template <class T, class InputArchive>
    struct has_minimal_input_serialization : std::integral_constant<bool,
      is_specialized_member_load_minimal<T, InputArchive>::value ||
      ((has_member_load_minimal<T, InputArchive>::value ||
        has_non_member_load_minimal<T, InputArchive>::value ||
        has_member_versioned_load_minimal<T, InputArchive>::value ||
        has_non_member_versioned_load_minimal<T, InputArchive>::value) &&
       (!is_specialized_member_serialize<T, InputArchive>::value ||
        !is_specialized_member_load<T, InputArchive>::value))> {};

    // ######################################################################
    namespace detail
    {
      struct base_class_id
      {
        template<class T>
          base_class_id(T const * const t) :
          type(typeid(T)),
          ptr(t),
          hash(std::hash<std::type_index>()(typeid(T)) ^ (std::hash<void const *>()(t) << 1))
          { }

          bool operator==(base_class_id const & other) const
          { return (type == other.type) && (ptr == other.ptr); }

          std::type_index type;
          void const * ptr;
          size_t hash;
      };
      struct base_class_id_hash { size_t operator()(base_class_id const & id) const { return id.hash; }  };
    } // namespace detail

    // ######################################################################
    //! A macro to use to restrict which types of archives your function will work for.
    /*! This requires you to have a template class parameter named Archive and replaces the void return
        type for your function.

        INTYPE refers to the input archive type you wish to restrict on.
        OUTTYPE refers to the output archive type you wish to restrict on.

        For example, if we want to limit a serialize to only work with binary serialization:

        @code{.cpp}
        template <class Archive>
        CEREAL_ARCHIVE_RESTRICT(BinaryInputArchive, BinaryOutputArchive)
        serialize( Archive & ar, MyCoolType & m )
        {
          ar & m;
        }
        @endcode

        If you need to do more restrictions in your enable_if, you will need to do this by hand.
     */
    #define CEREAL_ARCHIVE_RESTRICT(INTYPE, OUTTYPE) \
    typename std::enable_if<std::is_same<Archive, INTYPE>::value || std::is_same<Archive, OUTTYPE>::value, void>::type

    // ######################################################################
    namespace detail
    {
      struct shared_from_this_wrapper
      {
        template <class U>
        static auto check( U const & t ) -> decltype( ::cereal::access::shared_from_this(t), std::true_type() );

        static auto check( ... ) -> decltype( std::false_type() );

        template <class U>
        static auto get( U const & t ) -> decltype( t.shared_from_this() );
      };
    }

    //! Determine if T or any base class of T has inherited from std::enable_shared_from_this
    template<class T>
    struct has_shared_from_this : decltype(detail::shared_from_this_wrapper::check(std::declval<T>()))
    { };

    //! Get the type of the base class of T which inherited from std::enable_shared_from_this
    template <class T>
    struct get_shared_from_this_base
    {
      private:
        using PtrType = decltype(detail::shared_from_this_wrapper::get(std::declval<T>()));
      public:
        //! The type of the base of T that inherited from std::enable_shared_from_this
        using type = typename std::decay<typename PtrType::element_type>::type;
    };

    // ######################################################################
    //! Extracts the true type from something possibly wrapped in a cereal NoConvert
    /*! Internally cereal uses some wrapper classes to test the validity of non-member
        minimal load and save functions.  This can interfere with user type traits on
        templated load and save minimal functions.  To get to the correct underlying type,
        users should use strip_minimal when performing any enable_if type type trait checks.

        See the enum serialization in types/common.hpp for an example of using this */
    template <class T, bool IsCerealMinimalTrait = std::is_base_of<detail::NoConvertBase, T>::value>
    struct strip_minimal
    {
      using type = T;
    };

    //! Specialization for types wrapped in a NoConvert
    template <class T>
    struct strip_minimal<T, true>
    {
      using type = typename T::type;
    };
  } // namespace traits

  // ######################################################################
  namespace detail
  {
    template <class T, class A, bool Member = traits::has_member_load_and_construct<T, A>::value, bool NonMember = traits::has_non_member_load_and_construct<T, A>::value>
    struct Construct
    {
      static_assert( cereal::traits::detail::delay_static_assert<T>::value,
        "Cereal detected both member and non member load_and_construct functions!" );
      static T * load_andor_construct( A & /*ar*/, construct<T> & /*construct*/ )
      { return nullptr; }
    };

    template <class T, class A>
    struct Construct<T, A, false, false>
    {
      static_assert( std::is_default_constructible<T>::value,
                     "Trying to serialize a an object with no default constructor. \n\n "
                     "Types must either be default constructible or define either a member or non member Construct function. \n "
                     "Construct functions generally have the signature: \n\n "
                     "template <class Archive> \n "
                     "static void load_and_construct(Archive & ar, cereal::construct<T> & construct) \n "
                     "{ \n "
                     "  var a; \n "
                     "  ar( a ) \n "
                     "  construct( a ); \n "
                     "} \n\n" );
      static T * load_andor_construct()
      { return new T(); }
    };

    template <class T, class A>
    struct Construct<T, A, true, false>
    {
      static void load_andor_construct( A & ar, construct<T> & construct )
      {
        access::load_and_construct<T>( ar, construct );
      }
    };

    template <class T, class A>
    struct Construct<T, A, false, true>
    {
      static void load_andor_construct( A & ar, construct<T> & construct )
      {
        LoadAndConstruct<T>::load_and_construct( ar, construct );
      }
    };
  } // namespace detail
} // namespace cereal

#endif // CEREAL_DETAILS_TRAITS_HPP_
