/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2001-2010 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CHAR_CLASS_NOVEMBER_10_2006_0907AM)
#define BOOST_SPIRIT_CHAR_CLASS_NOVEMBER_10_2006_0907AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <string>

#include <boost/proto/proto.hpp>
#include <boost/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <boost/type_traits/make_unsigned.hpp>
#include <boost/type_traits/make_signed.hpp>

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable: 4800) // 'int' : forcing value to bool 'true' or 'false' warning
#endif

namespace boost { namespace spirit { namespace detail
{
    // Here's the thing... typical encodings (except ASCII) deal with unsigned
    // integers > 127. ASCII uses only 127. Yet, most char and wchar_t are signed.
    // Thus, a char with value > 127 is negative (e.g. char 233 is -23). When you
    // cast this to an unsigned int with 32 bits, you get 4294967273!
    //
    // The trick is to cast to an unsigned version of the source char first
    // before casting to the target. {P.S. Don't worry about the code, the
    // optimizer will optimize the if-else branches}

    template <typename TargetChar, typename SourceChar>
    TargetChar cast_char(SourceChar ch)
    {
        if (is_signed<TargetChar>::value != is_signed<SourceChar>::value)
        {
            if (is_signed<SourceChar>::value)
            {
                 // source is signed, target is unsigned
                typedef typename make_unsigned<SourceChar>::type USourceChar;
                return TargetChar(USourceChar(ch));
            }
            else
            {
                 // source is unsigned, target is signed
                typedef typename make_signed<SourceChar>::type SSourceChar;
                return TargetChar(SSourceChar(ch));
            }
        }
        else
        {
            // source and target has same signedness
            return TargetChar(ch); // just cast
        }
    }
}}}

namespace boost { namespace spirit { namespace tag
{
    struct char_ {};
    struct string {};

    ///////////////////////////////////////////////////////////////////////////
    // classification tags
    struct alnum {};
    struct alpha {};
    struct digit {};
    struct xdigit {};
    struct cntrl {};
    struct graph {};
    struct print {};
    struct punct {};
    struct space {};
    struct blank {};

    ///////////////////////////////////////////////////////////////////////////
    // classification/conversion tags
    struct no_case {};
    struct lower {};
    struct upper {};
    struct lowernum {};
    struct uppernum {};
    struct ucs4 {};
    struct encoding {};

#if defined(BOOST_SPIRIT_UNICODE)
///////////////////////////////////////////////////////////////////////////
//  Unicode Major Categories
///////////////////////////////////////////////////////////////////////////
    struct letter {};
    struct mark {};
    struct number {};
    struct separator {};
    struct other {};
    struct punctuation {};
    struct symbol {};

///////////////////////////////////////////////////////////////////////////
//  Unicode General Categories
///////////////////////////////////////////////////////////////////////////
    struct uppercase_letter {};
    struct lowercase_letter {};
    struct titlecase_letter {};
    struct modifier_letter {};
    struct other_letter {};

    struct nonspacing_mark {};
    struct enclosing_mark {};
    struct spacing_mark {};

    struct decimal_number {};
    struct letter_number {};
    struct other_number {};

    struct space_separator {};
    struct line_separator {};
    struct paragraph_separator {};

    struct control {};
    struct format {};
    struct private_use {};
    struct surrogate {};
    struct unassigned {};

    struct dash_punctuation {};
    struct open_punctuation {};
    struct close_punctuation {};
    struct connector_punctuation {};
    struct other_punctuation {};
    struct initial_punctuation {};
    struct final_punctuation {};

    struct math_symbol {};
    struct currency_symbol {};
    struct modifier_symbol {};
    struct other_symbol {};

///////////////////////////////////////////////////////////////////////////
//  Unicode Derived Categories
///////////////////////////////////////////////////////////////////////////
    struct alphabetic {};
    struct uppercase {};
    struct lowercase {};
    struct white_space {};
    struct hex_digit {};
    struct noncharacter_code_point {};
    struct default_ignorable_code_point {};

///////////////////////////////////////////////////////////////////////////
//  Unicode Scripts
///////////////////////////////////////////////////////////////////////////
    struct arabic {};
    struct imperial_aramaic {};
    struct armenian {};
    struct avestan {};
    struct balinese {};
    struct bamum {};
    struct bengali {};
    struct bopomofo {};
    struct braille {};
    struct buginese {};
    struct buhid {};
    struct canadian_aboriginal {};
    struct carian {};
    struct cham {};
    struct cherokee {};
    struct coptic {};
    struct cypriot {};
    struct cyrillic {};
    struct devanagari {};
    struct deseret {};
    struct egyptian_hieroglyphs {};
    struct ethiopic {};
    struct georgian {};
    struct glagolitic {};
    struct gothic {};
    struct greek {};
    struct gujarati {};
    struct gurmukhi {};
    struct hangul {};
    struct han {};
    struct hanunoo {};
    struct hebrew {};
    struct hiragana {};
    struct katakana_or_hiragana {};
    struct old_italic {};
    struct javanese {};
    struct kayah_li {};
    struct katakana {};
    struct kharoshthi {};
    struct khmer {};
    struct kannada {};
    struct kaithi {};
    struct tai_tham {};
    struct lao {};
    struct latin {};
    struct lepcha {};
    struct limbu {};
    struct linear_b {};
    struct lisu {};
    struct lycian {};
    struct lydian {};
    struct malayalam {};
    struct mongolian {};
    struct meetei_mayek {};
    struct myanmar {};
    struct nko {};
    struct ogham {};
    struct ol_chiki {};
    struct old_turkic {};
    struct oriya {};
    struct osmanya {};
    struct phags_pa {};
    struct inscriptional_pahlavi {};
    struct phoenician {};
    struct inscriptional_parthian {};
    struct rejang {};
    struct runic {};
    struct samaritan {};
    struct old_south_arabian {};
    struct saurashtra {};
    struct shavian {};
    struct sinhala {};
    struct sundanese {};
    struct syloti_nagri {};
    struct syriac {};
    struct tagbanwa {};
    struct tai_le {};
    struct new_tai_lue {};
    struct tamil {};
    struct tai_viet {};
    struct telugu {};
    struct tifinagh {};
    struct tagalog {};
    struct thaana {};
    struct thai {};
    struct tibetan {};
    struct ugaritic {};
    struct vai {};
    struct old_persian {};
    struct cuneiform {};
    struct yi {};
    struct inherited {};
    struct common {};
    struct unknown {};
#endif

    ///////////////////////////////////////////////////////////////////////////
    // This composite tag type encodes both the character
    // set and the specific char tag (used for classification
    // or conversion). char_code_base and char_encoding_base
    // can be used to test for modifier membership (see modifier.hpp)
    template <typename CharClass>
    struct char_code_base {};

    template <typename CharEncoding>
    struct char_encoding_base {};

    template <typename CharClass, typename CharEncoding>
    struct char_code
        : char_code_base<CharClass>, char_encoding_base<CharEncoding>
    {
        typedef CharEncoding char_encoding; // e.g. ascii
        typedef CharClass char_class;       // e.g. tag::alnum
    };

}}}

namespace boost { namespace spirit { namespace char_class
{
    ///////////////////////////////////////////////////////////////////////////
    // Test characters for classification
    template <typename CharEncoding>
    struct classify
    {
        typedef typename CharEncoding::char_type char_type;

#define BOOST_SPIRIT_CLASSIFY(name, isname)                                     \
        template <typename Char>                                                \
        static bool                                                             \
        is(tag::name, Char ch)                                                  \
        {                                                                       \
            return CharEncoding::isname                                         \
                BOOST_PREVENT_MACRO_SUBSTITUTION                                \
                    (detail::cast_char<char_type>(ch));                         \
        }                                                                       \
        /***/

        BOOST_SPIRIT_CLASSIFY(char_, ischar)
        BOOST_SPIRIT_CLASSIFY(alnum, isalnum)
        BOOST_SPIRIT_CLASSIFY(alpha, isalpha)
        BOOST_SPIRIT_CLASSIFY(digit, isdigit)
        BOOST_SPIRIT_CLASSIFY(xdigit, isxdigit)
        BOOST_SPIRIT_CLASSIFY(cntrl, iscntrl)
        BOOST_SPIRIT_CLASSIFY(graph, isgraph)
        BOOST_SPIRIT_CLASSIFY(lower, islower)
        BOOST_SPIRIT_CLASSIFY(print, isprint)
        BOOST_SPIRIT_CLASSIFY(punct, ispunct)
        BOOST_SPIRIT_CLASSIFY(space, isspace)
        BOOST_SPIRIT_CLASSIFY(blank, isblank)
        BOOST_SPIRIT_CLASSIFY(upper, isupper)

#undef BOOST_SPIRIT_CLASSIFY

        template <typename Char>
        static bool
        is(tag::lowernum, Char ch)
        {
            return CharEncoding::islower(detail::cast_char<char_type>(ch)) ||
                   CharEncoding::isdigit(detail::cast_char<char_type>(ch));
        }

        template <typename Char>
        static bool
        is(tag::uppernum, Char ch)
        {
            return CharEncoding::isupper(detail::cast_char<char_type>(ch)) ||
                   CharEncoding::isdigit(detail::cast_char<char_type>(ch));
        }

#if defined(BOOST_SPIRIT_UNICODE)

#define BOOST_SPIRIT_UNICODE_CLASSIFY(name)                                     \
        template <typename Char>                                                \
        static bool                                                             \
        is(tag::name, Char ch)                                                  \
        {                                                                       \
            return CharEncoding::is_##name(detail::cast_char<char_type>(ch));   \
        }                                                                       \
        /***/

///////////////////////////////////////////////////////////////////////////
//  Unicode Major Categories
///////////////////////////////////////////////////////////////////////////
    BOOST_SPIRIT_UNICODE_CLASSIFY(letter)
    BOOST_SPIRIT_UNICODE_CLASSIFY(mark)
    BOOST_SPIRIT_UNICODE_CLASSIFY(number)
    BOOST_SPIRIT_UNICODE_CLASSIFY(separator)
    BOOST_SPIRIT_UNICODE_CLASSIFY(other)
    BOOST_SPIRIT_UNICODE_CLASSIFY(punctuation)
    BOOST_SPIRIT_UNICODE_CLASSIFY(symbol)

///////////////////////////////////////////////////////////////////////////
//  Unicode General Categories
///////////////////////////////////////////////////////////////////////////
    BOOST_SPIRIT_UNICODE_CLASSIFY(uppercase_letter)
    BOOST_SPIRIT_UNICODE_CLASSIFY(lowercase_letter)
    BOOST_SPIRIT_UNICODE_CLASSIFY(titlecase_letter)
    BOOST_SPIRIT_UNICODE_CLASSIFY(modifier_letter)
    BOOST_SPIRIT_UNICODE_CLASSIFY(other_letter)

    BOOST_SPIRIT_UNICODE_CLASSIFY(nonspacing_mark)
    BOOST_SPIRIT_UNICODE_CLASSIFY(enclosing_mark)
    BOOST_SPIRIT_UNICODE_CLASSIFY(spacing_mark)

    BOOST_SPIRIT_UNICODE_CLASSIFY(decimal_number)
    BOOST_SPIRIT_UNICODE_CLASSIFY(letter_number)
    BOOST_SPIRIT_UNICODE_CLASSIFY(other_number)

    BOOST_SPIRIT_UNICODE_CLASSIFY(space_separator)
    BOOST_SPIRIT_UNICODE_CLASSIFY(line_separator)
    BOOST_SPIRIT_UNICODE_CLASSIFY(paragraph_separator)

    BOOST_SPIRIT_UNICODE_CLASSIFY(control)
    BOOST_SPIRIT_UNICODE_CLASSIFY(format)
    BOOST_SPIRIT_UNICODE_CLASSIFY(private_use)
    BOOST_SPIRIT_UNICODE_CLASSIFY(surrogate)
    BOOST_SPIRIT_UNICODE_CLASSIFY(unassigned)

    BOOST_SPIRIT_UNICODE_CLASSIFY(dash_punctuation)
    BOOST_SPIRIT_UNICODE_CLASSIFY(open_punctuation)
    BOOST_SPIRIT_UNICODE_CLASSIFY(close_punctuation)
    BOOST_SPIRIT_UNICODE_CLASSIFY(connector_punctuation)
    BOOST_SPIRIT_UNICODE_CLASSIFY(other_punctuation)
    BOOST_SPIRIT_UNICODE_CLASSIFY(initial_punctuation)
    BOOST_SPIRIT_UNICODE_CLASSIFY(final_punctuation)

    BOOST_SPIRIT_UNICODE_CLASSIFY(math_symbol)
    BOOST_SPIRIT_UNICODE_CLASSIFY(currency_symbol)
    BOOST_SPIRIT_UNICODE_CLASSIFY(modifier_symbol)
    BOOST_SPIRIT_UNICODE_CLASSIFY(other_symbol)

///////////////////////////////////////////////////////////////////////////
//  Unicode Derived Categories
///////////////////////////////////////////////////////////////////////////
    BOOST_SPIRIT_UNICODE_CLASSIFY(alphabetic)
    BOOST_SPIRIT_UNICODE_CLASSIFY(uppercase)
    BOOST_SPIRIT_UNICODE_CLASSIFY(lowercase)
    BOOST_SPIRIT_UNICODE_CLASSIFY(white_space)
    BOOST_SPIRIT_UNICODE_CLASSIFY(hex_digit)
    BOOST_SPIRIT_UNICODE_CLASSIFY(noncharacter_code_point)
    BOOST_SPIRIT_UNICODE_CLASSIFY(default_ignorable_code_point)

///////////////////////////////////////////////////////////////////////////
//  Unicode Scripts
///////////////////////////////////////////////////////////////////////////
    BOOST_SPIRIT_UNICODE_CLASSIFY(arabic)
    BOOST_SPIRIT_UNICODE_CLASSIFY(imperial_aramaic)
    BOOST_SPIRIT_UNICODE_CLASSIFY(armenian)
    BOOST_SPIRIT_UNICODE_CLASSIFY(avestan)
    BOOST_SPIRIT_UNICODE_CLASSIFY(balinese)
    BOOST_SPIRIT_UNICODE_CLASSIFY(bamum)
    BOOST_SPIRIT_UNICODE_CLASSIFY(bengali)
    BOOST_SPIRIT_UNICODE_CLASSIFY(bopomofo)
    BOOST_SPIRIT_UNICODE_CLASSIFY(braille)
    BOOST_SPIRIT_UNICODE_CLASSIFY(buginese)
    BOOST_SPIRIT_UNICODE_CLASSIFY(buhid)
    BOOST_SPIRIT_UNICODE_CLASSIFY(canadian_aboriginal)
    BOOST_SPIRIT_UNICODE_CLASSIFY(carian)
    BOOST_SPIRIT_UNICODE_CLASSIFY(cham)
    BOOST_SPIRIT_UNICODE_CLASSIFY(cherokee)
    BOOST_SPIRIT_UNICODE_CLASSIFY(coptic)
    BOOST_SPIRIT_UNICODE_CLASSIFY(cypriot)
    BOOST_SPIRIT_UNICODE_CLASSIFY(cyrillic)
    BOOST_SPIRIT_UNICODE_CLASSIFY(devanagari)
    BOOST_SPIRIT_UNICODE_CLASSIFY(deseret)
    BOOST_SPIRIT_UNICODE_CLASSIFY(egyptian_hieroglyphs)
    BOOST_SPIRIT_UNICODE_CLASSIFY(ethiopic)
    BOOST_SPIRIT_UNICODE_CLASSIFY(georgian)
    BOOST_SPIRIT_UNICODE_CLASSIFY(glagolitic)
    BOOST_SPIRIT_UNICODE_CLASSIFY(gothic)
    BOOST_SPIRIT_UNICODE_CLASSIFY(greek)
    BOOST_SPIRIT_UNICODE_CLASSIFY(gujarati)
    BOOST_SPIRIT_UNICODE_CLASSIFY(gurmukhi)
    BOOST_SPIRIT_UNICODE_CLASSIFY(hangul)
    BOOST_SPIRIT_UNICODE_CLASSIFY(han)
    BOOST_SPIRIT_UNICODE_CLASSIFY(hanunoo)
    BOOST_SPIRIT_UNICODE_CLASSIFY(hebrew)
    BOOST_SPIRIT_UNICODE_CLASSIFY(hiragana)
    BOOST_SPIRIT_UNICODE_CLASSIFY(katakana_or_hiragana)
    BOOST_SPIRIT_UNICODE_CLASSIFY(old_italic)
    BOOST_SPIRIT_UNICODE_CLASSIFY(javanese)
    BOOST_SPIRIT_UNICODE_CLASSIFY(kayah_li)
    BOOST_SPIRIT_UNICODE_CLASSIFY(katakana)
    BOOST_SPIRIT_UNICODE_CLASSIFY(kharoshthi)
    BOOST_SPIRIT_UNICODE_CLASSIFY(khmer)
    BOOST_SPIRIT_UNICODE_CLASSIFY(kannada)
    BOOST_SPIRIT_UNICODE_CLASSIFY(kaithi)
    BOOST_SPIRIT_UNICODE_CLASSIFY(tai_tham)
    BOOST_SPIRIT_UNICODE_CLASSIFY(lao)
    BOOST_SPIRIT_UNICODE_CLASSIFY(latin)
    BOOST_SPIRIT_UNICODE_CLASSIFY(lepcha)
    BOOST_SPIRIT_UNICODE_CLASSIFY(limbu)
    BOOST_SPIRIT_UNICODE_CLASSIFY(linear_b)
    BOOST_SPIRIT_UNICODE_CLASSIFY(lisu)
    BOOST_SPIRIT_UNICODE_CLASSIFY(lycian)
    BOOST_SPIRIT_UNICODE_CLASSIFY(lydian)
    BOOST_SPIRIT_UNICODE_CLASSIFY(malayalam)
    BOOST_SPIRIT_UNICODE_CLASSIFY(mongolian)
    BOOST_SPIRIT_UNICODE_CLASSIFY(meetei_mayek)
    BOOST_SPIRIT_UNICODE_CLASSIFY(myanmar)
    BOOST_SPIRIT_UNICODE_CLASSIFY(nko)
    BOOST_SPIRIT_UNICODE_CLASSIFY(ogham)
    BOOST_SPIRIT_UNICODE_CLASSIFY(ol_chiki)
    BOOST_SPIRIT_UNICODE_CLASSIFY(old_turkic)
    BOOST_SPIRIT_UNICODE_CLASSIFY(oriya)
    BOOST_SPIRIT_UNICODE_CLASSIFY(osmanya)
    BOOST_SPIRIT_UNICODE_CLASSIFY(phags_pa)
    BOOST_SPIRIT_UNICODE_CLASSIFY(inscriptional_pahlavi)
    BOOST_SPIRIT_UNICODE_CLASSIFY(phoenician)
    BOOST_SPIRIT_UNICODE_CLASSIFY(inscriptional_parthian)
    BOOST_SPIRIT_UNICODE_CLASSIFY(rejang)
    BOOST_SPIRIT_UNICODE_CLASSIFY(runic)
    BOOST_SPIRIT_UNICODE_CLASSIFY(samaritan)
    BOOST_SPIRIT_UNICODE_CLASSIFY(old_south_arabian)
    BOOST_SPIRIT_UNICODE_CLASSIFY(saurashtra)
    BOOST_SPIRIT_UNICODE_CLASSIFY(shavian)
    BOOST_SPIRIT_UNICODE_CLASSIFY(sinhala)
    BOOST_SPIRIT_UNICODE_CLASSIFY(sundanese)
    BOOST_SPIRIT_UNICODE_CLASSIFY(syloti_nagri)
    BOOST_SPIRIT_UNICODE_CLASSIFY(syriac)
    BOOST_SPIRIT_UNICODE_CLASSIFY(tagbanwa)
    BOOST_SPIRIT_UNICODE_CLASSIFY(tai_le)
    BOOST_SPIRIT_UNICODE_CLASSIFY(new_tai_lue)
    BOOST_SPIRIT_UNICODE_CLASSIFY(tamil)
    BOOST_SPIRIT_UNICODE_CLASSIFY(tai_viet)
    BOOST_SPIRIT_UNICODE_CLASSIFY(telugu)
    BOOST_SPIRIT_UNICODE_CLASSIFY(tifinagh)
    BOOST_SPIRIT_UNICODE_CLASSIFY(tagalog)
    BOOST_SPIRIT_UNICODE_CLASSIFY(thaana)
    BOOST_SPIRIT_UNICODE_CLASSIFY(thai)
    BOOST_SPIRIT_UNICODE_CLASSIFY(tibetan)
    BOOST_SPIRIT_UNICODE_CLASSIFY(ugaritic)
    BOOST_SPIRIT_UNICODE_CLASSIFY(vai)
    BOOST_SPIRIT_UNICODE_CLASSIFY(old_persian)
    BOOST_SPIRIT_UNICODE_CLASSIFY(cuneiform)
    BOOST_SPIRIT_UNICODE_CLASSIFY(yi)
    BOOST_SPIRIT_UNICODE_CLASSIFY(inherited)
    BOOST_SPIRIT_UNICODE_CLASSIFY(common)
    BOOST_SPIRIT_UNICODE_CLASSIFY(unknown)

#undef BOOST_SPIRIT_UNICODE_CLASSIFY
#endif

    };

    ///////////////////////////////////////////////////////////////////////////
    // Convert characters
    template <typename CharEncoding>
    struct convert
    {
        typedef typename CharEncoding::char_type char_type;

        template <typename Char>
        static Char
        to(tag::lower, Char ch)
        {
            return static_cast<Char>(
                CharEncoding::tolower(detail::cast_char<char_type>(ch)));
        }

        template <typename Char>
        static Char
        to(tag::upper, Char ch)
        {
            return static_cast<Char>(
                CharEncoding::toupper(detail::cast_char<char_type>(ch)));
        }

        template <typename Char>
        static Char
        to(tag::ucs4, Char ch)
        {
            return static_cast<Char>(
                CharEncoding::toucs4(detail::cast_char<char_type>(ch)));
        }

        template <typename Char>
        static Char
        to(unused_type, Char ch)
        {
            return ch;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Info on character classification
    template <typename CharEncoding>
    struct what
    {
#define BOOST_SPIRIT_CLASSIFY_WHAT(name, isname)                                \
        static char const* is(tag::name)                                        \
        {                                                                       \
            return isname;                                                      \
        }                                                                       \
        /***/

        BOOST_SPIRIT_CLASSIFY_WHAT(char_, "char")
        BOOST_SPIRIT_CLASSIFY_WHAT(alnum, "alnum")
        BOOST_SPIRIT_CLASSIFY_WHAT(alpha, "alpha")
        BOOST_SPIRIT_CLASSIFY_WHAT(digit, "digit")
        BOOST_SPIRIT_CLASSIFY_WHAT(xdigit, "xdigit")
        BOOST_SPIRIT_CLASSIFY_WHAT(cntrl, "cntrl")
        BOOST_SPIRIT_CLASSIFY_WHAT(graph, "graph")
        BOOST_SPIRIT_CLASSIFY_WHAT(lower, "lower")
        BOOST_SPIRIT_CLASSIFY_WHAT(lowernum, "lowernum")
        BOOST_SPIRIT_CLASSIFY_WHAT(print, "print")
        BOOST_SPIRIT_CLASSIFY_WHAT(punct, "punct")
        BOOST_SPIRIT_CLASSIFY_WHAT(space, "space")
        BOOST_SPIRIT_CLASSIFY_WHAT(blank, "blank")
        BOOST_SPIRIT_CLASSIFY_WHAT(upper, "upper")
        BOOST_SPIRIT_CLASSIFY_WHAT(uppernum, "uppernum")
        BOOST_SPIRIT_CLASSIFY_WHAT(ucs4, "ucs4")

#undef BOOST_SPIRIT_CLASSIFY_WHAT

#if defined(BOOST_SPIRIT_UNICODE)

#define BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(name)                                \
        static char const* is(tag::name)                                        \
        {                                                                       \
            return BOOST_PP_STRINGIZE(name);                                    \
        }                                                                       \
        /***/

///////////////////////////////////////////////////////////////////////////
//  Unicode Major Categories
///////////////////////////////////////////////////////////////////////////
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(letter)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(mark)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(number)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(separator)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(other)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(punctuation)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(symbol)

///////////////////////////////////////////////////////////////////////////
//  Unicode General Categories
///////////////////////////////////////////////////////////////////////////
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(uppercase_letter)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(lowercase_letter)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(titlecase_letter)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(modifier_letter)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(other_letter)

    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(nonspacing_mark)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(enclosing_mark)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(spacing_mark)

    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(decimal_number)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(letter_number)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(other_number)

    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(space_separator)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(line_separator)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(paragraph_separator)

    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(control)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(format)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(private_use)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(surrogate)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(unassigned)

    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(dash_punctuation)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(open_punctuation)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(close_punctuation)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(connector_punctuation)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(other_punctuation)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(initial_punctuation)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(final_punctuation)

    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(math_symbol)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(currency_symbol)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(modifier_symbol)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(other_symbol)

///////////////////////////////////////////////////////////////////////////
//  Unicode Derived Categories
///////////////////////////////////////////////////////////////////////////
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(alphabetic)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(uppercase)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(lowercase)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(white_space)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(hex_digit)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(noncharacter_code_point)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(default_ignorable_code_point)

///////////////////////////////////////////////////////////////////////////
//  Unicode Scripts
///////////////////////////////////////////////////////////////////////////
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(arabic)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(imperial_aramaic)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(armenian)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(avestan)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(balinese)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(bamum)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(bengali)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(bopomofo)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(braille)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(buginese)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(buhid)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(canadian_aboriginal)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(carian)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(cham)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(cherokee)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(coptic)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(cypriot)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(cyrillic)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(devanagari)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(deseret)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(egyptian_hieroglyphs)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(ethiopic)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(georgian)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(glagolitic)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(gothic)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(greek)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(gujarati)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(gurmukhi)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(hangul)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(han)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(hanunoo)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(hebrew)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(hiragana)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(katakana_or_hiragana)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(old_italic)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(javanese)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(kayah_li)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(katakana)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(kharoshthi)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(khmer)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(kannada)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(kaithi)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(tai_tham)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(lao)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(latin)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(lepcha)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(limbu)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(linear_b)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(lisu)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(lycian)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(lydian)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(malayalam)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(mongolian)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(meetei_mayek)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(myanmar)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(nko)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(ogham)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(ol_chiki)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(old_turkic)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(oriya)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(osmanya)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(phags_pa)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(inscriptional_pahlavi)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(phoenician)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(inscriptional_parthian)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(rejang)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(runic)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(samaritan)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(old_south_arabian)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(saurashtra)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(shavian)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(sinhala)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(sundanese)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(syloti_nagri)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(syriac)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(tagbanwa)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(tai_le)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(new_tai_lue)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(tamil)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(tai_viet)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(telugu)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(tifinagh)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(tagalog)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(thaana)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(thai)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(tibetan)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(ugaritic)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(vai)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(old_persian)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(cuneiform)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(yi)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(inherited)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(common)
    BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT(unknown)

#undef BOOST_SPIRIT_UNICODE_CLASSIFY_WHAT
#endif

    };
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // This meta-function evaluates to mpl::true_ if the function
    // char_encoding::ischar() needs to be called to ensure correct matching.
    // This happens mainly if the character type returned from the underlying
    // iterator is larger than the character type of the used character
    // encoding. Additionally, this meta-function provides a customization
    // point for the lexer library to enforce this behavior while parsing
    // a token stream.
    template <typename Char, typename BaseChar>
    struct mustcheck_ischar
      : mpl::bool_<(sizeof(Char) > sizeof(BaseChar)) ? true : false> {};

    ///////////////////////////////////////////////////////////////////////////
    // The following template calls char_encoding::ischar, if necessary
    template <typename CharParam, typename CharEncoding
      , bool MustCheck = mustcheck_ischar<
            CharParam, typename CharEncoding::char_type>::value>
    struct ischar
    {
        static bool call(CharParam)
        {
            return true;
        }
    };

    template <typename CharParam, typename CharEncoding>
    struct ischar<CharParam, CharEncoding, true>
    {
        static bool call(CharParam const& ch)
        {
            return CharEncoding::ischar(int(ch));
        }
    };

}}}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#endif


