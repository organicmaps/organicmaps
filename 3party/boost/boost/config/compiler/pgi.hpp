//  (C) Copyright Noel Belcourt 2007.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version.

//  PGI C++ compiler setup:

#define BOOST_COMPILER_VERSION __PGIC__##__PGIC_MINOR__
#define BOOST_COMPILER "PGI compiler version " BOOST_STRINGIZE(_COMPILER_VERSION)

//
// Threading support:
// Turn this on unconditionally here, it will get turned off again later
// if no threading API is detected.
//

// PGI 10.x doesn't seem to define __PGIC__

// versions earlier than 10.x do define __PGIC__
#if __PGIC__ >= 10

// options requested by configure --enable-test
#define BOOST_HAS_PTHREADS
#define BOOST_HAS_NRVO
#define BOOST_HAS_LONG_LONG

// options --enable-test wants undefined
#undef BOOST_NO_STDC_NAMESPACE
#undef BOOST_NO_EXCEPTION_STD_NAMESPACE
#undef BOOST_DEDUCED_TYPENAME

#elif __PGIC__ >= 7

#define BOOST_FUNCTION_SCOPE_USING_DECLARATION_BREAKS_ADL 
#define BOOST_NO_TWO_PHASE_NAME_LOOKUP
#define BOOST_NO_SWPRINTF
#define BOOST_NO_AUTO_MULTIDECLARATIONS
#define BOOST_NO_AUTO_DECLARATIONS

#else

#  error "Pgi compiler not configured - please reconfigure"

#endif
//
// C++0x features
//
//   See boost\config\suffix.hpp for BOOST_NO_LONG_LONG
//
#define BOOST_NO_CHAR16_T
#define BOOST_NO_CHAR32_T
#define BOOST_NO_CONCEPTS
#define BOOST_NO_CONSTEXPR
#define BOOST_NO_DECLTYPE
#define BOOST_NO_DEFAULTED_FUNCTIONS
#define BOOST_NO_DELETED_FUNCTIONS
#define BOOST_NO_EXPLICIT_CONVERSION_OPERATORS
#define BOOST_NO_EXTERN_TEMPLATE
#define BOOST_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS
#define BOOST_NO_INITIALIZER_LISTS
#define BOOST_NO_LAMBDAS
#define BOOST_NO_NULLPTR
#define BOOST_NO_RAW_LITERALS
#define BOOST_NO_RVALUE_REFERENCES
#define BOOST_NO_SCOPED_ENUMS
#define BOOST_NO_SFINAE_EXPR
#define BOOST_NO_STATIC_ASSERT
#define BOOST_NO_TEMPLATE_ALIASES
#define BOOST_NO_UNICODE_LITERALS
#define BOOST_NO_VARIADIC_TEMPLATES
#define BOOST_NO_VARIADIC_MACROS

//
// version check:
// probably nothing to do here?

