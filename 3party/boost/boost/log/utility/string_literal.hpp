/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   string_literal.hpp
 * \author Andrey Semashev
 * \date   24.06.2007
 *
 * The header contains implementation of a constant string literal wrapper.
 */

#ifndef BOOST_LOG_UTILITY_STRING_LITERAL_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_STRING_LITERAL_HPP_INCLUDED_

#include <cstddef>
#include <stdexcept>
#include <iosfwd>
#include <string>
#include <iterator>
#include <boost/operators.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/utility/string_literal_fwd.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_LOG_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * \brief String literal wrapper
 *
 * The \c basic_string_literal is a thin wrapper around a constant string literal.
 * It provides interface similar to STL strings, but because of read-only nature
 * of string literals, lacks ability to modify string contents. However,
 * \c basic_string_literal objects can be assigned to and cleared.
 *
 * The main advantage of this class comparing to other string classes is that
 * it doesn't dynamically allocate memory and therefore is fast, thin and exception safe.
 */
template< typename CharT, typename TraitsT >
class basic_string_literal
    //! \cond
    : public totally_ordered1< basic_string_literal< CharT, TraitsT >,
        totally_ordered2< basic_string_literal< CharT, TraitsT >, const CharT*,
            totally_ordered2<
                basic_string_literal< CharT, TraitsT >,
                std::basic_string< CharT, TraitsT >
            >
        >
    >
    //! \endcond
{
    //! Self type
    typedef basic_string_literal< CharT, TraitsT > this_type;

public:
    typedef CharT value_type;
    typedef TraitsT traits_type;

    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef const value_type* const_pointer;
    typedef value_type const& const_reference;
    typedef const value_type* const_iterator;
    typedef std::reverse_iterator< const_iterator > const_reverse_iterator;

    //! Corresponding STL string type
    typedef std::basic_string< value_type, traits_type > string_type;

private:
    //! Pointer to the beginning of the literal
    const_pointer m_pStart;
    //! Length
    size_type m_Len;

    //! Empty string literal to support clear
    static const value_type g_EmptyString[1];

public:
    /*!
     * Constructor
     *
     * \post <tt>empty() == true</tt>
     */
    basic_string_literal() { clear(); }

    /*!
     * Constructor from a string literal
     *
     * \post <tt>*this == p</tt>
     * \param p A zero-terminated constant sequence of characters
     */
    template< typename T, size_type LenV >
    basic_string_literal(T(&p)[LenV]
        //! \cond
        , typename enable_if< is_same< T, const value_type >, int >::type = 0
        //! \endcond
        )
        : m_pStart(p), m_Len(LenV - 1)
    {
    }

    /*!
     * Copy constructor
     *
     * \post <tt>*this == that</tt>
     * \param that Source literal to copy string from
     */
    basic_string_literal(basic_string_literal const& that) : m_pStart(that.m_pStart), m_Len(that.m_Len) {}

    /*!
     * Assignment operator
     *
     * \post <tt>*this == that</tt>
     * \param that Source literal to copy string from
     */
    this_type& operator= (this_type const& that)
    {
        return assign(that);
    }
    /*!
     * Assignment from a string literal
     *
     * \post <tt>*this == p</tt>
     * \param p A zero-terminated constant sequence of characters
     */
    template< typename T, size_type LenV >
#ifndef BOOST_LOG_DOXYGEN_PASS
    typename enable_if<
        is_same< T, const value_type >,
        this_type&
    >::type
#else
    this_type&
#endif // BOOST_LOG_DOXYGEN_PASS
    operator= (T(&p)[LenV])
    {
        return assign(p);
    }

    /*!
     * Lexicographical comparison (equality)
     *
     * \param that Comparand
     * \return \c true if the comparand string equals to this string, \c false otherwise
     */
    bool operator== (this_type const& that) const
    {
        return (compare_internal(m_pStart, m_Len, that.m_pStart, that.m_Len) == 0);
    }
    /*!
     * Lexicographical comparison (equality)
     *
     * \param str Comparand. Must point to a zero-terminated sequence of characters, must not be NULL.
     * \return \c true if the comparand string equals to this string, \c false otherwise
     */
    bool operator== (const_pointer str) const
    {
        return (compare_internal(m_pStart, m_Len, str, traits_type::length(str)) == 0);
    }
    /*!
     * Lexicographical comparison (equality)
     *
     * \param that Comparand
     * \return \c true if the comparand string equals to this string, \c false otherwise
     */
    bool operator== (string_type const& that) const
    {
        return (compare_internal(m_pStart, m_Len, that.c_str(), that.size()) == 0);
    }

    /*!
     * Lexicographical comparison (less ordering)
     *
     * \param that Comparand
     * \return \c true if this string is less than the comparand, \c false otherwise
     */
    bool operator< (this_type const& that) const
    {
        return (compare_internal(m_pStart, m_Len, that.m_pStart, that.m_Len) < 0);
    }
    /*!
     * Lexicographical comparison (less ordering)
     *
     * \param str Comparand. Must point to a zero-terminated sequence of characters, must not be NULL.
     * \return \c true if this string is less than the comparand, \c false otherwise
     */
    bool operator< (const_pointer str) const
    {
        return (compare_internal(m_pStart, m_Len, str, traits_type::length(str)) < 0);
    }
    /*!
     * Lexicographical comparison (less ordering)
     *
     * \param that Comparand
     * \return \c true if this string is less than the comparand, \c false otherwise
     */
    bool operator< (string_type const& that) const
    {
        return (compare_internal(m_pStart, m_Len, that.c_str(), that.size()) < 0);
    }

    /*!
     * Lexicographical comparison (greater ordering)
     *
     * \param that Comparand
     * \return \c true if this string is greater than the comparand, \c false otherwise
     */
    bool operator> (this_type const& that) const
    {
        return (compare_internal(m_pStart, m_Len, that.m_pStart, that.m_Len) > 0);
    }
    /*!
     * Lexicographical comparison (greater ordering)
     *
     * \param str Comparand. Must point to a zero-terminated sequence of characters, must not be NULL.
     * \return \c true if this string is greater than the comparand, \c false otherwise
     */
    bool operator> (const_pointer str) const
    {
        return (compare_internal(m_pStart, m_Len, str, traits_type::length(str)) > 0);
    }
    /*!
     * Lexicographical comparison (greater ordering)
     *
     * \param that Comparand
     * \return \c true if this string is greater than the comparand, \c false otherwise
     */
    bool operator> (string_type const& that) const
    {
        return (compare_internal(m_pStart, m_Len, that.c_str(), that.size()) > 0);
    }

    /*!
     * Subscript operator
     *
     * \pre <tt>i < size()</tt>
     * \param i Requested character index
     * \return Constant reference to the requested character
     */
    const_reference operator[] (size_type i) const
    {
        return m_pStart[i];
    }
    /*!
     * Checked subscript
     *
     * \param i Requested character index
     * \return Constant reference to the requested character
     *
     * \b Throws: An <tt>std::exception</tt>-based exception if index \a i is out of string boundaries
     */
    const_reference at(size_type i) const
    {
        if (i >= m_Len)
            BOOST_THROW_EXCEPTION(std::out_of_range("basic_string_literal::at: the index value is out of range"));
        return m_pStart[i];
    }

    /*!
     * \return Pointer to the beginning of the literal
     */
    const_pointer c_str() const { return m_pStart; }
    /*!
     * \return Pointer to the beginning of the literal
     */
    const_pointer data() const { return m_pStart; }
    /*!
     * \return Length of the literal
     */
    size_type size() const { return m_Len; }
    /*!
     * \return Length of the literal
     */
    size_type length() const { return m_Len; }

    /*!
     * \return \c true if the literal is an empty string, \c false otherwise
     */
    bool empty() const
    {
        return (m_Len == 0);
    }

    /*!
     * \return Iterator that points to the first character of the literal
     */
    const_iterator begin() const { return m_pStart; }
    /*!
     * \return Iterator that points after the last character of the literal
     */
    const_iterator end() const { return m_pStart + m_Len; }
    /*!
     * \return Reverse iterator that points to the last character of the literal
     */
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    /*!
     * \return Reverse iterator that points before the first character of the literal
     */
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    /*!
     * \return STL string constructed from the literal
     */
    string_type str() const
    {
        return string_type(m_pStart, m_Len);
    }

    /*!
     * The method clears the literal
     *
     * \post <tt>empty() == true</tt>
     */
    void clear()
    {
        m_pStart = g_EmptyString;
        m_Len = 0;
    }
    /*!
     * The method swaps two literals
     */
    void swap(this_type& that)
    {
        register const_pointer p = m_pStart;
        m_pStart = that.m_pStart;
        that.m_pStart = p;

        register size_type l = m_Len;
        m_Len = that.m_Len;
        that.m_Len = l;
    }

    /*!
     * Assignment from another literal
     *
     * \post <tt>*this == that</tt>
     * \param that Source literal to copy string from
     */
    this_type& assign(this_type const& that)
    {
        m_pStart = that.m_pStart;
        m_Len = that.m_Len;
        return *this;
    }
    /*!
     * Assignment from another literal
     *
     * \post <tt>*this == p</tt>
     * \param p A zero-terminated constant sequence of characters
     */
    template< typename T, size_type LenV >
#ifndef BOOST_LOG_DOXYGEN_PASS
    typename enable_if<
        is_same< T, const value_type >,
        this_type&
    >::type
#else
    this_type&
#endif // BOOST_LOG_DOXYGEN_PASS
    assign(T(&p)[LenV])
    {
        m_pStart = p;
        m_Len = LenV - 1;
        return *this;
    }

    /*!
     * The method copies the literal or its portion to an external buffer
     *
     * \pre <tt>pos <= size()</tt>
     * \param str Pointer to the external buffer beginning. Must not be NULL.
     *            The buffer must have enough capacity to accommodate the requested number of characters.
     * \param n Maximum number of characters to copy
     * \param pos Starting position to start copying from
     * \return Number of characters copied
     *
     * \b Throws: An <tt>std::exception</tt>-based exception if \a pos is out of range.
     */
    size_type copy(value_type* str, size_type n, size_type pos = 0) const
    {
        if (pos > m_Len)
            BOOST_THROW_EXCEPTION(std::out_of_range("basic_string_literal::copy: the position is out of range"));

        register size_type len = m_Len - pos;
        if (len > n)
            len = n;
        traits_type::copy(str, m_pStart + pos, len);
        return len;
    }

    /*!
     * Lexicographically compares the argument string to a part of this string
     *
     * \pre <tt>pos <= size()</tt>
     * \param pos Starting position within this string to perform comparison to
     * \param n Length of the substring of this string to perform comparison to
     * \param str Comparand. Must point to a sequence of characters, must not be NULL.
     * \param len Number of characters in the sequence \a str.
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     *
     * \b Throws: An <tt>std::exception</tt>-based exception if \a pos is out of range.
     */
    int compare(size_type pos, size_type n, const_pointer str, size_type len) const
    {
        if (pos > m_Len)
            BOOST_THROW_EXCEPTION(std::out_of_range("basic_string_literal::compare: the position is out of range"));

        register size_type compare_size = m_Len - pos;
        if (compare_size > len)
            compare_size = len;
        if (compare_size > n)
            compare_size = n;
        return compare_internal(m_pStart + pos, compare_size, str, compare_size);
    }
    /*!
     * Lexicographically compares the argument string to a part of this string
     *
     * \pre <tt>pos <= size()</tt>
     * \param pos Starting position within this string to perform comparison to
     * \param n Length of the substring of this string to perform comparison to
     * \param str Comparand. Must point to a zero-terminated sequence of characters, must not be NULL.
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     *
     * \b Throws: An <tt>std::exception</tt>-based exception if \a pos is out of range.
     */
    int compare(size_type pos, size_type n, const_pointer str) const
    {
        return compare(pos, n, str, traits_type::length(str));
    }
    /*!
     * Lexicographically compares the argument string literal to a part of this string
     *
     * \pre <tt>pos <= size()</tt>
     * \param pos Starting position within this string to perform comparison to
     * \param n Length of the substring of this string to perform comparison to
     * \param that Comparand
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     *
     * \b Throws: An <tt>std::exception</tt>-based exception if \a pos is out of range.
     */
    int compare(size_type pos, size_type n, this_type const& that) const
    {
        return compare(pos, n, that.c_str(), that.size());
    }
    /*!
     * Lexicographically compares the argument string to this string
     *
     * \param str Comparand. Must point to a sequence of characters, must not be NULL.
     * \param len Number of characters in the sequence \a str.
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     */
    int compare(const_pointer str, size_type len) const
    {
        return compare(0, m_Len, str, len);
    }
    /*!
     * Lexicographically compares the argument string to this string
     *
     * \param str Comparand. Must point to a zero-terminated sequence of characters, must not be NULL.
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     */
    int compare(const_pointer str) const
    {
        return compare(0, m_Len, str, traits_type::length(str));
    }
    /*!
     * Lexicographically compares the argument string to this string
     *
     * \param that Comparand
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     */
    int compare(this_type const& that) const
    {
        return compare(0, m_Len, that.c_str(), that.size());
    }

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! Internal comparison implementation
    static int compare_internal(const_pointer pLeft, size_type LeftLen, const_pointer pRight, size_type RightLen)
    {
        if (pLeft != pRight)
        {
            register const int result = traits_type::compare(
                pLeft, pRight, (LeftLen < RightLen ? LeftLen : RightLen));
            if (result != 0)
                return result;
        }
        return static_cast< int >(LeftLen - RightLen);
    }
#endif // BOOST_LOG_DOXYGEN_PASS
};

template< typename CharT, typename TraitsT >
typename basic_string_literal< CharT, TraitsT >::value_type const
basic_string_literal< CharT, TraitsT >::g_EmptyString[1] = { 0 };

//! Output operator
template< typename CharT, typename StrmTraitsT, typename LitTraitsT >
inline std::basic_ostream< CharT, StrmTraitsT >& operator<< (
    std::basic_ostream< CharT, StrmTraitsT >& strm, basic_string_literal< CharT, LitTraitsT > const& lit)
{
    strm.write(lit.c_str(), static_cast< std::streamsize >(lit.size()));
    return strm;
}

//! External swap
template< typename CharT, typename TraitsT >
inline void swap(
    basic_string_literal< CharT, TraitsT >& left,
    basic_string_literal< CharT, TraitsT >& right)
{
    left.swap(right);
}

//! Creates a string literal wrapper from a constant string literal
#ifdef BOOST_LOG_USE_CHAR
template< typename T, std::size_t LenV >
inline
#ifndef BOOST_LOG_DOXYGEN_PASS
typename enable_if<
    is_same< T, const char >,
    string_literal
>::type
#else
basic_string_literal< T >
#endif // BOOST_LOG_DOXYGEN_PASS
str_literal(T(&p)[LenV])
{
    return string_literal(p);
}
#endif

#ifndef BOOST_LOG_DOXYGEN_PASS

#ifdef BOOST_LOG_USE_WCHAR_T
template< typename T, std::size_t LenV >
inline typename enable_if<
    is_same< T, const wchar_t >,
    wstring_literal
>::type
str_literal(T(&p)[LenV])
{
    return wstring_literal(p);
}
#endif

#endif // BOOST_LOG_DOXYGEN_PASS

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_STRING_LITERAL_HPP_INCLUDED_
