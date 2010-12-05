// (C) Copyright Douglas Gregor 2010
//
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version.

// Clang compiler setup.

#if __has_feature(cxx_exceptions) && !defined(BOOST_NO_EXCEPTIONS)
#else
#  define BOOST_NO_EXCEPTIONS
#endif

#if __has_feature(cxx_rtti)
#else
#  define BOOST_NO_RTTI
#endif

#if defined(__int64)
#  define BOOST_HAS_MS_INT64
#endif

#define BOOST_HAS_NRVO

// NOTE: Clang's C++0x support is not worth detecting. However, it
// supports both extern templates and "long long" even in C++98/03
// mode.
#define BOOST_NO_AUTO_DECLARATIONS
#define BOOST_NO_AUTO_MULTIDECLARATIONS
#define BOOST_NO_CHAR16_T
#define BOOST_NO_CHAR32_T
#define BOOST_NO_CONCEPTS
#define BOOST_NO_CONSTEXPR
#define BOOST_NO_DECLTYPE
#define BOOST_NO_DEFAULTED_FUNCTIONS
#define BOOST_NO_DELETED_FUNCTIONS
#define BOOST_NO_EXPLICIT_CONVERSION_OPERATORS
#define BOOST_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS
#define BOOST_NO_INITIALIZER_LISTS
#define BOOST_NO_LAMBDAS
#define BOOST_NO_NULLPTR
#define BOOST_NO_RAW_LITERALS
#define BOOST_NO_RVALUE_REFERENCES
#define BOOST_NO_SCOPED_ENUMS
#define BOOST_NO_STATIC_ASSERT
#define BOOST_NO_TEMPLATE_ALIASES
#define BOOST_NO_UNICODE_LITERALS
#define BOOST_NO_VARIADIC_TEMPLATES

// HACK: Clang does support extern templates, but Boost's test for
// them is wrong.
#define BOOST_NO_EXTERN_TEMPLATE

#ifndef BOOST_COMPILER
#  define BOOST_COMPILER "Clang version " __clang_version__
#endif

// Macro used to identify the Clang compiler.
#define BOOST_CLANG 1

