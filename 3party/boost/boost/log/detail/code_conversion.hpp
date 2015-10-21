/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   code_conversion.hpp
 * \author Andrey Semashev
 * \date   08.11.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_CODE_CONVERSION_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_CODE_CONVERSION_HPP_INCLUDED_

#include <cstddef>
#include <locale>
#include <string>
#include <boost/core/enable_if.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/is_character_type.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert_impl(const wchar_t* str1, std::size_t len, std::string& str2, std::locale const& loc = std::locale());
//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert_impl(const char* str1, std::size_t len, std::wstring& str2, std::locale const& loc = std::locale());

#if !defined(BOOST_LOG_NO_CXX11_CODECVT_FACETS)
#if !defined(BOOST_NO_CXX11_CHAR16_T)
//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert_impl(const char16_t* str1, std::size_t len, std::string& str2, std::locale const& loc = std::locale());
//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert_impl(const char* str1, std::size_t len, std::u16string& str2, std::locale const& loc = std::locale());
//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert_impl(const char16_t* str1, std::size_t len, std::wstring& str2, std::locale const& loc = std::locale());
#endif
#if !defined(BOOST_NO_CXX11_CHAR32_T)
//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert_impl(const char32_t* str1, std::size_t len, std::string& str2, std::locale const& loc = std::locale());
//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert_impl(const char* str1, std::size_t len, std::u32string& str2, std::locale const& loc = std::locale());
//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert_impl(const char32_t* str1, std::size_t len, std::wstring& str2, std::locale const& loc = std::locale());
#endif
#if !defined(BOOST_NO_CXX11_CHAR16_T) && !defined(BOOST_NO_CXX11_CHAR32_T)
//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert_impl(const char16_t* str1, std::size_t len, std::u32string& str2, std::locale const& loc = std::locale());
//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert_impl(const char32_t* str1, std::size_t len, std::u16string& str2, std::locale const& loc = std::locale());
#endif
#endif // !defined(BOOST_LOG_NO_CXX11_CODECVT_FACETS)

//! The function converts one string to the character type of another
template< typename SourceCharT, typename SourceTraitsT, typename SourceAllocatorT, typename TargetCharT, typename TargetTraitsT, typename TargetAllocatorT >
inline typename boost::enable_if_c< is_character_type< SourceCharT >::value && is_character_type< TargetCharT >::value && sizeof(SourceCharT) == sizeof(TargetCharT) >::type
code_convert(std::basic_string< SourceCharT, SourceTraitsT, SourceAllocatorT > const& str1, std::basic_string< TargetCharT, TargetTraitsT, TargetAllocatorT >& str2, std::locale const& = std::locale())
{
    str2.append(reinterpret_cast< const TargetCharT* >(str1.c_str()), str1.size());
}
//! The function converts one string to the character type of another
template< typename SourceCharT, typename TargetCharT, typename TargetTraitsT, typename TargetAllocatorT >
inline typename boost::enable_if_c< is_character_type< SourceCharT >::value && is_character_type< TargetCharT >::value && sizeof(SourceCharT) == sizeof(TargetCharT) >::type
code_convert(const SourceCharT* str1, std::size_t len, std::basic_string< TargetCharT, TargetTraitsT, TargetAllocatorT >& str2, std::locale const& = std::locale())
{
    str2.append(reinterpret_cast< const TargetCharT* >(str1), len);
}

//! The function converts one string to the character type of another
template< typename SourceCharT, typename SourceTraitsT, typename SourceAllocatorT, typename TargetCharT, typename TargetTraitsT, typename TargetAllocatorT >
inline typename boost::enable_if_c< is_character_type< SourceCharT >::value && is_character_type< TargetCharT >::value && sizeof(SourceCharT) != sizeof(TargetCharT) >::type
code_convert(std::basic_string< SourceCharT, SourceTraitsT, SourceAllocatorT > const& str1, std::basic_string< TargetCharT, TargetTraitsT, TargetAllocatorT >& str2, std::locale const& loc = std::locale())
{
    aux::code_convert_impl(str1.c_str(), str1.size(), str2, loc);
}

//! The function converts one string to the character type of another
template< typename SourceCharT, typename TargetCharT, typename TargetTraitsT, typename TargetAllocatorT >
inline typename boost::enable_if_c< is_character_type< SourceCharT >::value && is_character_type< TargetCharT >::value && sizeof(SourceCharT) != sizeof(TargetCharT) >::type
code_convert(const SourceCharT* str1, std::size_t len, std::basic_string< TargetCharT, TargetTraitsT, TargetAllocatorT >& str2, std::locale const& loc = std::locale())
{
    aux::code_convert_impl(str1, len, str2, loc);
}

//! The function converts the passed string to the narrow-character encoding
inline std::string const& to_narrow(std::string const& str)
{
    return str;
}

//! The function converts the passed string to the narrow-character encoding
inline std::string const& to_narrow(std::string const& str, std::locale const&)
{
    return str;
}

//! The function converts the passed string to the narrow-character encoding
inline std::string to_narrow(std::wstring const& str, std::locale const& loc = std::locale())
{
    std::string res;
    aux::code_convert(str, res, loc);
    return res;
}

//! The function converts the passed string to the wide-character encoding
inline std::wstring const& to_wide(std::wstring const& str)
{
    return str;
}

//! The function converts the passed string to the wide-character encoding
inline std::wstring const& to_wide(std::wstring const& str, std::locale const&)
{
    return str;
}

//! The function converts the passed string to the wide-character encoding
inline std::wstring to_wide(std::string const& str, std::locale const& loc = std::locale())
{
    std::wstring res;
    aux::code_convert(str, res, loc);
    return res;
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_CODE_CONVERSION_HPP_INCLUDED_
