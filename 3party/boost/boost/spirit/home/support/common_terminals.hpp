/*=============================================================================
  Copyright (c) 2001-2011 Joel de Guzman
  http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_COMMON_PLACEHOLDERS_OCTOBER_16_2008_0102PM
#define BOOST_SPIRIT_COMMON_PLACEHOLDERS_OCTOBER_16_2008_0102PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/terminal.hpp>
#include <boost/spirit/home/support/char_encoding/standard.hpp>
#include <boost/spirit/home/support/char_encoding/standard_wide.hpp>
#include <boost/spirit/home/support/char_encoding/ascii.hpp>
#include <boost/spirit/home/support/char_encoding/iso8859_1.hpp>
#include <boost/spirit/home/support/char_class.hpp>
#include <boost/mpl/vector.hpp>

#if defined(BOOST_SPIRIT_UNICODE)
# include <boost/spirit/home/support/char_encoding/unicode.hpp>
#endif

namespace boost { namespace spirit
{
    typedef mpl::vector<
            spirit::char_encoding::ascii
          , spirit::char_encoding::iso8859_1
          , spirit::char_encoding::standard
          , spirit::char_encoding::standard_wide
#if defined(BOOST_SPIRIT_UNICODE)
          , spirit::char_encoding::unicode
#endif
        >
    char_encodings;

    template <typename T>
    struct is_char_encoding : mpl::false_ {};

    template <>
    struct is_char_encoding<spirit::char_encoding::ascii> : mpl::true_ {};

    template <>
    struct is_char_encoding<spirit::char_encoding::iso8859_1> : mpl::true_ {};

    template <>
    struct is_char_encoding<spirit::char_encoding::standard> : mpl::true_ {};

    template <>
    struct is_char_encoding<spirit::char_encoding::standard_wide> : mpl::true_ {};

#if defined(BOOST_SPIRIT_UNICODE)
    template <>
    struct is_char_encoding<spirit::char_encoding::unicode> : mpl::true_ {};
#endif

    template <typename Encoding>
    struct encoding
        : proto::terminal<tag::char_code<tag::encoding, Encoding> >::type
    {};

    // Our basic terminals
    BOOST_SPIRIT_DEFINE_TERMINALS(
        ( verbatim )
        ( no_delimit )
        ( lexeme )
        ( no_skip )
        ( omit )
        ( raw )
        ( as_string )
        ( as_wstring )
        ( inf )
        ( eol )
        ( eoi )
        ( buffer )
        ( true_ )
        ( false_ )
        ( matches )
        ( hold )
        ( strict )
        ( relaxed )
        ( duplicate )
    )

    // Our extended terminals
    BOOST_SPIRIT_DEFINE_TERMINALS_EX(
        ( lit )
        ( bin )
        ( oct )
        ( hex )
        ( bool_ )
        ( ushort_ )
        ( ulong_ )
        ( uint_ )
        ( short_ )
        ( long_ )
        ( int_ )
        ( ulong_long )
        ( long_long )
        ( float_ )
        ( double_ )
        ( long_double )
        ( repeat )
        ( eps )
        ( pad )
        ( byte_ )
        ( word )
        ( big_word )
        ( little_word )
        ( dword )
        ( big_dword )
        ( little_dword )
        ( qword )
        ( big_qword )
        ( little_qword )
        ( skip )
        ( delimit )
        ( stream )
        ( wstream )
        ( left_align )
        ( right_align )
        ( center )
        ( maxwidth )
        ( set_state )
        ( in_state )
        ( token )
        ( tokenid )
        ( attr )
        ( columns )
        ( auto_ )
    )

    // special tags (used mainly for stateful tag types)
    namespace tag
    {
        struct attr_cast {};
        struct as {};
    }
}}

///////////////////////////////////////////////////////////////////////////////
// Here we place the character-set sensitive placeholders. We have one set
// each for ascii, iso8859_1, standard and standard_wide and unicode. These
// placeholders are placed in its char-set namespace. For example, there exist
// a placeholder spirit::ascii::alnum for ascii versions of alnum.

#define BOOST_SPIRIT_TAG_CHAR_SPEC(charset)                                     \
    typedef tag::char_code<tag::char_, charset> char_;                          \
    typedef tag::char_code<tag::string, charset> string;                        \
    /***/

#define BOOST_SPIRIT_CHAR_SPEC(charset)                                         \
    typedef spirit::terminal<tag::charset::char_> char_type;                    \
    char_type const char_ = char_type();                                        \
                                                                                \
    inline void silence_unused_warnings_##char_() { (void) char_; }             \
                                                                                \
    typedef spirit::terminal<tag::charset::string> string_type;                 \
    string_type const string = string_type();                                   \
                                                                                \
    inline void silence_unused_warnings_##string() { (void) string; }           \
    /***/

#define BOOST_SPIRIT_CHAR_CODE(name, charset)                                   \
    typedef proto::terminal<tag::char_code<tag::name, charset> >::type          \
        name##_type;                                                            \
    name##_type const name = name##_type();                                     \
                                                                                \
    inline void silence_unused_warnings_##name() { (void) name; }               \
    /***/

#define BOOST_SPIRIT_DEFINE_CHAR_CODES(charset)                                 \
    namespace boost { namespace spirit { namespace tag { namespace charset      \
    {                                                                           \
        BOOST_SPIRIT_TAG_CHAR_SPEC(spirit::char_encoding::charset)              \
    }}}}                                                                        \
    namespace boost { namespace spirit { namespace charset                      \
    {                                                                           \
        BOOST_SPIRIT_CHAR_SPEC(charset)                                         \
                                                                                \
        BOOST_SPIRIT_CHAR_CODE(alnum, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(alpha, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(blank, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(cntrl, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(digit, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(graph, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(print, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(punct, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(space, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(xdigit, spirit::char_encoding::charset)          \
                                                                                \
        BOOST_SPIRIT_CHAR_CODE(no_case, spirit::char_encoding::charset)         \
        BOOST_SPIRIT_CHAR_CODE(lower, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(upper, spirit::char_encoding::charset)           \
        BOOST_SPIRIT_CHAR_CODE(lowernum, spirit::char_encoding::charset)        \
        BOOST_SPIRIT_CHAR_CODE(uppernum, spirit::char_encoding::charset)        \
    }}}                                                                         \
    /***/

BOOST_SPIRIT_DEFINE_CHAR_CODES(ascii)
BOOST_SPIRIT_DEFINE_CHAR_CODES(iso8859_1)
BOOST_SPIRIT_DEFINE_CHAR_CODES(standard)
BOOST_SPIRIT_DEFINE_CHAR_CODES(standard_wide)

namespace boost { namespace spirit { namespace traits
{
    template <typename Char>
    struct char_encoding_from_char;

    template <>
    struct char_encoding_from_char<char>
      : mpl::identity<spirit::char_encoding::standard>
    {};

    template <>
    struct char_encoding_from_char<wchar_t>
      : mpl::identity<spirit::char_encoding::standard_wide>
    {};

    template <typename T>
    struct char_encoding_from_char<T const>
      : char_encoding_from_char<T>
    {};
}}}

#if defined(BOOST_SPIRIT_UNICODE)
BOOST_SPIRIT_DEFINE_CHAR_CODES(unicode)

    namespace boost { namespace spirit { namespace tag { namespace unicode
    {
        BOOST_SPIRIT_TAG_CHAR_SPEC(spirit::char_encoding::unicode)
    }}}}

    namespace boost { namespace spirit { namespace unicode
    {
#define BOOST_SPIRIT_UNICODE_CHAR_CODE(name)                                    \
    BOOST_SPIRIT_CHAR_CODE(name, spirit::char_encoding::unicode)                \

    ///////////////////////////////////////////////////////////////////////////
    //  Unicode Major Categories
    ///////////////////////////////////////////////////////////////////////////
        BOOST_SPIRIT_UNICODE_CHAR_CODE(letter)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(mark)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(number)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(separator)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(other)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(punctuation)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(symbol)

    ///////////////////////////////////////////////////////////////////////////
    //  Unicode General Categories
    ///////////////////////////////////////////////////////////////////////////
        BOOST_SPIRIT_UNICODE_CHAR_CODE(uppercase_letter)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(lowercase_letter)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(titlecase_letter)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(modifier_letter)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(other_letter)

        BOOST_SPIRIT_UNICODE_CHAR_CODE(nonspacing_mark)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(enclosing_mark)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(spacing_mark)

        BOOST_SPIRIT_UNICODE_CHAR_CODE(decimal_number)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(letter_number)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(other_number)

        BOOST_SPIRIT_UNICODE_CHAR_CODE(space_separator)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(line_separator)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(paragraph_separator)

        BOOST_SPIRIT_UNICODE_CHAR_CODE(control)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(format)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(private_use)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(surrogate)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(unassigned)

        BOOST_SPIRIT_UNICODE_CHAR_CODE(dash_punctuation)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(open_punctuation)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(close_punctuation)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(connector_punctuation)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(other_punctuation)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(initial_punctuation)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(final_punctuation)

        BOOST_SPIRIT_UNICODE_CHAR_CODE(math_symbol)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(currency_symbol)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(modifier_symbol)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(other_symbol)

    ///////////////////////////////////////////////////////////////////////////
    //  Unicode Derived Categories
    ///////////////////////////////////////////////////////////////////////////
        BOOST_SPIRIT_UNICODE_CHAR_CODE(alphabetic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(uppercase)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(lowercase)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(white_space)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(hex_digit)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(noncharacter_code_point)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(default_ignorable_code_point)

    ///////////////////////////////////////////////////////////////////////////
    //  Unicode Scripts
    ///////////////////////////////////////////////////////////////////////////
        BOOST_SPIRIT_UNICODE_CHAR_CODE(arabic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(imperial_aramaic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(armenian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(avestan)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(balinese)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(bamum)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(bengali)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(bopomofo)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(braille)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(buginese)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(buhid)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(canadian_aboriginal)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(carian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(cham)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(cherokee)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(coptic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(cypriot)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(cyrillic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(devanagari)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(deseret)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(egyptian_hieroglyphs)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(ethiopic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(georgian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(glagolitic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(gothic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(greek)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(gujarati)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(gurmukhi)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(hangul)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(han)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(hanunoo)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(hebrew)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(hiragana)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(katakana_or_hiragana)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(old_italic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(javanese)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(kayah_li)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(katakana)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(kharoshthi)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(khmer)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(kannada)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(kaithi)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tai_tham)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(lao)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(latin)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(lepcha)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(limbu)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(linear_b)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(lisu)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(lycian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(lydian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(malayalam)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(mongolian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(meetei_mayek)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(myanmar)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(nko)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(ogham)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(ol_chiki)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(old_turkic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(oriya)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(osmanya)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(phags_pa)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(inscriptional_pahlavi)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(phoenician)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(inscriptional_parthian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(rejang)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(runic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(samaritan)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(old_south_arabian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(saurashtra)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(shavian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(sinhala)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(sundanese)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(syloti_nagri)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(syriac)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tagbanwa)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tai_le)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(new_tai_lue)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tamil)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tai_viet)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(telugu)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tifinagh)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tagalog)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(thaana)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(thai)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(tibetan)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(ugaritic)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(vai)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(old_persian)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(cuneiform)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(yi)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(inherited)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(common)
        BOOST_SPIRIT_UNICODE_CHAR_CODE(unknown)

#undef BOOST_SPIRIT_UNICODE_CHAR_CODE
    }}}
#endif

#endif
