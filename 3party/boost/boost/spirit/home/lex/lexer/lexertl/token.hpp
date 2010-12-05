//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_TOKEN_FEB_10_2008_0751PM)
#define BOOST_SPIRIT_LEX_TOKEN_FEB_10_2008_0751PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/spirit/home/qi/detail/assign_to.hpp>
#include <boost/spirit/home/support/attributes.hpp>
#include <boost/spirit/home/support/argument.hpp>
#include <boost/spirit/home/support/detail/lexer/generator.hpp>
#include <boost/spirit/home/support/detail/lexer/rules.hpp>
#include <boost/spirit/home/support/detail/lexer/consts.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/value_at.hpp>
#include <boost/detail/iterator.hpp>
#include <boost/variant.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/if.hpp>
#include <boost/range/iterator_range.hpp>
#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
#include <boost/static_assert.hpp>
#endif

#if defined(BOOST_SPIRIT_DEBUG)
#include <iosfwd>
#endif

namespace boost { namespace spirit { namespace lex { namespace lexertl
{ 
    ///////////////////////////////////////////////////////////////////////////
    //
    //  The token is the type of the objects returned by default by the 
    //  iterator.
    //
    //    template parameters:
    //        Iterator        The type of the iterator used to access the
    //                        underlying character stream.
    //        AttributeTypes  A mpl sequence containing the types of all 
    //                        required different token values to be supported 
    //                        by this token type.
    //        HasState        A mpl::bool_ indicating, whether this token type
    //                        should support lexer states.
    //
    //  It is possible to use other token types with the spirit::lex 
    //  framework as well. If you plan to use a different type as your token 
    //  type, you'll need to expose the following things from your token type 
    //  to make it compatible with spirit::lex:
    //
    //    typedefs
    //        iterator_type   The type of the iterator used to access the
    //                        underlying character stream.
    //
    //        id_type         The type of the token id used.
    //
    //    methods
    //        default constructor
    //                        This should initialize the token as an end of 
    //                        input token.
    //        constructors    The prototype of the other required 
    //                        constructors should be:
    //
    //              token(int)
    //                        This constructor should initialize the token as 
    //                        an invalid token (not carrying any specific 
    //                        values)
    //
    //              where:  the int is used as a tag only and its value is 
    //                      ignored
    //
    //                        and:
    //
    //              token(std::size_t id, std::size_t state, 
    //                    iterator_type first, iterator_type last);
    //
    //              where:  id:           token id
    //                      state:        lexer state this token was matched in
    //                      first, last:  pair of iterators marking the matched 
    //                                    range in the underlying input stream 
    //
    //        accessors
    //              id()      return the token id of the matched input sequence
    //              id(newid) set the token id of the token instance
    //
    //              state()   return the lexer state this token was matched in
    //
    //              value()   return the token value
    //
    //  Additionally, you will have to implement a couple of helper functions
    //  in the same namespace as the token type: a comparison operator==() to 
    //  compare your token instances, a token_is_valid() function and different 
    //  construct() function overloads as described below.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator = char const*
      , typename AttributeTypes = mpl::vector0<>
      , typename HasState = mpl::true_> 
    struct token;

    ///////////////////////////////////////////////////////////////////////////
    //  This specialization of the token type doesn't contain any item data and
    //  doesn't support working with lexer states.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    struct token<Iterator, lex::omit, mpl::false_>
    {
        typedef Iterator iterator_type;
        typedef mpl::false_ has_state;
        typedef std::size_t id_type;
        typedef unused_type token_value_type;

        //  default constructed tokens correspond to EOI tokens
        token() : id_(boost::lexer::npos) {}

        //  construct an invalid token
        explicit token(int) : id_(0) {}

        token(id_type id, std::size_t) : id_(id) {}

        token(id_type id, std::size_t, token_value_type)
          : id_(id) {}

#if defined(BOOST_SPIRIT_DEBUG)
        token(id_type id, std::size_t, Iterator const& first
              , Iterator const& last)
          : id_(id) 
          , matched_(first, last)
        {}
#else
        token(id_type id, std::size_t, Iterator const&, Iterator const&)
          : id_(id) 
        {}
#endif

        //  this default conversion operator is needed to allow the direct 
        //  usage of tokens in conjunction with the primitive parsers defined 
        //  in Qi
        operator id_type() const { return id_; }

        //  Retrieve or set the token id of this token instance. 
        id_type id() const { return id_; }
        void id(id_type newid) { id_ = newid; }

        std::size_t state() const { return 0; }   // always '0' (INITIAL state)

        bool is_valid() const 
        { 
            return 0 != id_ && id_type(boost::lexer::npos) != id_; 
        }

#if defined(BOOST_SPIRIT_DEBUG)
#if BOOST_WORKAROUND(BOOST_MSVC, == 1600)
        // workaround for MSVC10 which has problems copying a default 
        // constructed iterator_range
        token& operator= (token const& rhs)
        {
            return *this;
        }
#endif
        std::pair<Iterator, Iterator> matched_;
#endif

// works only starting MSVC V8
#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1400)
    private:
        struct dummy { void true_() {}; };
        typedef void (dummy::*safe_bool)();

    public:
        operator safe_bool() const { return is_valid() ? &dummy::true_ : 0; }
#endif

    protected:
        id_type id_;            // token id, 0 if nothing has been matched
    };

#if defined(BOOST_SPIRIT_DEBUG)
    template <typename Char, typename Traits, typename Iterator
      , typename AttributeTypes, typename HasState> 
    inline std::basic_ostream<Char, Traits>& 
    operator<< (std::basic_ostream<Char, Traits>& os
      , token<Iterator, AttributeTypes, HasState> const& t)
    {
        if (t) {
            Iterator end = t.matched_.second;
            for (Iterator it = t.matched_.first; it != end; ++it)
                os << *it;
        }
        else {
            os << "<invalid token>";
        }
        return os;
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    //  This specialization of the token type doesn't contain any item data but
    //  supports working with lexer states.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    struct token<Iterator, lex::omit, mpl::true_>
      : token<Iterator, lex::omit, mpl::false_>
    {
    private:
        typedef token<Iterator, lex::omit, mpl::false_> base_type;

    public:
        typedef typename base_type::id_type id_type;
        typedef Iterator iterator_type;
        typedef mpl::true_ has_state;
        typedef unused_type token_value_type;

        //  default constructed tokens correspond to EOI tokens
        token() : state_(boost::lexer::npos) {}

        //  construct an invalid token
        explicit token(int) : base_type(0), state_(boost::lexer::npos) {}

        token(id_type id, std::size_t state)
          : base_type(id, boost::lexer::npos), state_(state) {}

        token(id_type id, std::size_t state, token_value_type)
          : base_type(id, boost::lexer::npos, unused)
          , state_(state) {}

        token(id_type id, std::size_t state
              , Iterator const& first, Iterator const& last)
          : base_type(id, boost::lexer::npos, first, last)
          , state_(state) {}

        std::size_t state() const { return state_; }

    protected:
        std::size_t state_;      // lexer state this token was matched in
    };

    ///////////////////////////////////////////////////////////////////////////
    //  The generic version of the token type derives from the 
    //  specialization above and adds a single data member holding the item 
    //  data carried by the token instance.
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////
        //  Metafunction to calculate the type of the variant data item to be 
        //  stored with each token instance.
        //
        //  Note: The iterator pair needs to be the first type in the list of 
        //        types supported by the generated variant type (this is being 
        //        used to identify whether the stored data item in a particular 
        //        token instance needs to be converted from the pair of 
        //        iterators (see the first of the assign_to_attribute_from_value 
        //        specializations below).
        ///////////////////////////////////////////////////////////////////////
        template <typename IteratorPair, typename AttributeTypes>
        struct token_value_typesequence
        {
            typedef typename mpl::insert<
                AttributeTypes
              , typename mpl::begin<AttributeTypes>::type
              , IteratorPair
            >::type sequence_type;
            typedef typename make_variant_over<sequence_type>::type type;
        };

        ///////////////////////////////////////////////////////////////////////
        //  The type of the data item stored with a token instance is defined 
        //  by the template parameter 'AttributeTypes' and may be:
        //  
        //     lex::omit:         no data item is stored with the token 
        //                        instance (this is handled by the 
        //                        specializations of the token class
        //                        below)
        //     mpl::vector0<>:    each token instance stores a pair of 
        //                        iterators pointing to the matched input 
        //                        sequence
        //     mpl::vector<...>:  each token instance stores a variant being 
        //                        able to store the pair of iterators pointing 
        //                        to the matched input sequence, or any of the 
        //                        types a specified in the mpl::vector<>
        //
        //  All this is done to ensure the token type is as small (in terms 
        //  of its byte-size) as possible.
        ///////////////////////////////////////////////////////////////////////
        template <typename IteratorPair, typename AttributeTypes>
        struct token_value_type
          : mpl::eval_if<
                mpl::or_<
                    is_same<AttributeTypes, mpl::vector0<> >
                  , is_same<AttributeTypes, mpl::vector<> > >
              , mpl::identity<IteratorPair>
              , token_value_typesequence<IteratorPair, AttributeTypes> >
        {};
    }

    template <typename Iterator, typename AttributeTypes, typename HasState>
    struct token : token<Iterator, lex::omit, HasState>
    {
    private: // precondition assertions
#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
        BOOST_STATIC_ASSERT((mpl::is_sequence<AttributeTypes>::value || 
                            is_same<AttributeTypes, lex::omit>::value));
#endif
        typedef token<Iterator, lex::omit, HasState> base_type;

    protected: 
        //  If no additional token value types are given, the the token will 
        //  hold the plain pair of iterators pointing to the matched range
        //  in the underlying input sequence. Otherwise the token value is 
        //  stored as a variant and will again hold the pair of iterators but
        //  is able to hold any of the given data types as well. The conversion 
        //  from the iterator pair to the required data type is done when it is
        //  accessed for the first time.
        typedef iterator_range<Iterator> iterpair_type;

    public:
        typedef typename base_type::id_type id_type;
        typedef typename detail::token_value_type<
            iterpair_type, AttributeTypes
        >::type token_value_type;

        typedef Iterator iterator_type;

        //  default constructed tokens correspond to EOI tokens
        token() : value_(iterpair_type(iterator_type(), iterator_type())) {}

        //  construct an invalid token
        explicit token(int)
          : base_type(0)
          , value_(iterpair_type(iterator_type(), iterator_type())) {}

        token(id_type id, std::size_t state, token_value_type const& value)
          : base_type(id, state, value)
          , value_(value) {}

        token(id_type id, std::size_t state, Iterator const& first
              , Iterator const& last)
          : base_type(id, state, first, last)
          , value_(iterpair_type(first, last)) {}

        token_value_type& value() { return value_; }
        token_value_type const& value() const { return value_; }

#if BOOST_WORKAROUND(BOOST_MSVC, == 1600)
        // workaround for MSVC10 which has problems copying a default 
        // constructed iterator_range
        token& operator= (token const& rhs)
        {
            if (this != &rhs) 
            {
                this->base_type::operator=(static_cast<base_type const&>(rhs));
                if (this->id_ != boost::lexer::npos && this->id_ != 0) 
                    value_ = rhs.value_;
            }
            return *this;
        }
#endif

    protected:
        token_value_type value_; // token value, by default a pair of iterators
    };

    ///////////////////////////////////////////////////////////////////////////
    //  tokens are considered equal, if their id's match (these are unique)
    template <typename Iterator, typename AttributeTypes, typename HasState>
    inline bool 
    operator== (token<Iterator, AttributeTypes, HasState> const& lhs, 
                token<Iterator, AttributeTypes, HasState> const& rhs)
    {
        return lhs.id() == rhs.id();
    }

    ///////////////////////////////////////////////////////////////////////////
    //  This overload is needed by the multi_pass/functor_input_policy to 
    //  validate a token instance. It has to be defined in the same namespace 
    //  as the token class itself to allow ADL to find it.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename AttributeTypes, typename HasState>
    inline bool 
    token_is_valid(token<Iterator, AttributeTypes, HasState> const& t)
    {
        return t.is_valid();
    }

}}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    //  We have to provide specializations for the customization point
    //  assign_to_attribute_from_value allowing to extract the needed value 
    //  from the token. 
    ///////////////////////////////////////////////////////////////////////////

    //  This is called from the parse function of token_def if the token_def
    //  has been defined to carry a special attribute type
    template <typename Attribute, typename Iterator, typename AttributeTypes
      , typename HasState>
    struct assign_to_attribute_from_value<Attribute
      , lex::lexertl::token<Iterator, AttributeTypes, HasState> >
    {
        static void 
        call(lex::lexertl::token<Iterator, AttributeTypes, HasState> const& t
          , Attribute& attr)
        {
        //  The goal of this function is to avoid the conversion of the pair of
        //  iterators (to the matched character sequence) into the token value 
        //  of the required type being done more than once. For this purpose it 
        //  checks whether the stored value type is still the default one (pair 
        //  of iterators) and if yes, replaces the pair of iterators with the 
        //  converted value to be returned from subsequent calls.

            if (0 == t.value().which()) {
            //  first access to the token value
                typedef iterator_range<Iterator> iterpair_type;
                iterpair_type const& ip = get<iterpair_type>(t.value());

            // Interestingly enough we use the assign_to() framework defined in 
            // Spirit.Qi allowing to convert the pair of iterators to almost any 
            // required type (assign_to(), if available, uses the standard Spirit 
            // parsers to do the conversion).
                spirit::traits::assign_to(ip.begin(), ip.end(), attr);

            //  If you get an error during the compilation of the following 
            //  assignment expression, you probably forgot to list one or more 
            //  types used as token value types (in your token_def<...> 
            //  definitions) in your definition of the token class. I.e. any token 
            //  value type used for a token_def<...> definition has to be listed 
            //  during the declaration of the token type to use. For instance let's 
            //  assume we have two token_def's:
            //
            //      token_def<int> number; number = "...";
            //      token_def<std::string> identifier; identifier = "...";
            //
            //  Then you'll have to use the following token type definition 
            //  (assuming you are using the token class):
            //
            //      typedef mpl::vector<int, std::string> token_values;
            //      typedef token<base_iter_type, token_values> token_type;
            //
            //  where: base_iter_type is the iterator type used to expose the 
            //         underlying input stream.
            //
            //  This token_type has to be used as the second template parameter 
            //  to the lexer class:
            //
            //      typedef lexer<base_iter_type, token_type> lexer_type;
            //
            //  again, assuming you're using the lexer<> template for your 
            //  tokenization.

                typedef lex::lexertl::token<
                    Iterator, AttributeTypes, HasState> token_type;
                const_cast<token_type&>(t).value() = attr;   // re-assign value
            }
            else {
            // reuse the already assigned value
                spirit::traits::assign_to(get<Attribute>(t.value()), attr);
            }
        }
    };

    //  These are called from the parse function of token_def if the token type
    //  has no special attribute type assigned 
    template <typename Attribute, typename Iterator, typename HasState>
    struct assign_to_attribute_from_value<
        Attribute, lex::lexertl::token<Iterator, mpl::vector0<>, HasState> >
    {
        static void 
        call(lex::lexertl::token<Iterator, mpl::vector0<>, HasState> const& t
          , Attribute& attr)
        {
            //  The default type returned by the token_def parser component (if 
            //  it has no token value type assigned) is the pair of iterators 
            //  to the matched character sequence.
            spirit::traits::assign_to(t.value().begin(), t.value().end(), attr);
        }
    };

    // same as above but using mpl::vector<> instead of mpl::vector0<>
    template <typename Attribute, typename Iterator, typename HasState>
    struct assign_to_attribute_from_value<
        Attribute, lex::lexertl::token<Iterator, mpl::vector<>, HasState> >
    {
        static void 
        call(lex::lexertl::token<Iterator, mpl::vector<>, HasState> const& t
          , Attribute& attr)
        {
            //  The default type returned by the token_def parser component (if 
            //  it has no token value type assigned) is the pair of iterators 
            //  to the matched character sequence.
            spirit::traits::assign_to(t.value().begin(), t.value().end(), attr);
        }
    };

    //  This is called from the parse function of token_def if the token type
    //  has been explicitly omitted (i.e. no attribute value is used), which
    //  essentially means that every attribute gets initialized using default 
    //  constructed values.
    template <typename Attribute, typename Iterator, typename HasState>
    struct assign_to_attribute_from_value<
        Attribute, lex::lexertl::token<Iterator, lex::omit, HasState> >
    {
        static void 
        call(lex::lexertl::token<Iterator, lex::omit, HasState> const& t
          , Attribute& attr)
        {
            // do nothing
        }
    };

    //  This is called from the parse function of lexer_def_
    template <typename Iterator, typename AttributeTypes, typename HasState>
    struct assign_to_attribute_from_value<
        fusion::vector2<std::size_t, iterator_range<Iterator> >
      , lex::lexertl::token<Iterator, AttributeTypes, HasState> >
    {
        static void 
        call(lex::lexertl::token<Iterator, AttributeTypes, HasState> const& t
          , fusion::vector2<std::size_t, iterator_range<Iterator> >& attr)
        {
            //  The type returned by the lexer_def_ parser components is a 
            //  fusion::vector containing the token id of the matched token 
            //  and the pair of iterators to the matched character sequence.
            typedef iterator_range<Iterator> iterpair_type;
            typedef fusion::vector2<std::size_t, iterator_range<Iterator> > 
                attribute_type;

            iterpair_type const& ip = get<iterpair_type>(t.value());
            attr = attribute_type(t.id(), get<iterpair_type>(t.value()));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Overload debug output for a single token, this integrates lexer tokens 
    // with Qi's simple_trace debug facilities
    template <typename Iterator, typename Attribute, typename HasState>
    struct token_printer_debug<lex::lexertl::token<Iterator, Attribute, HasState> >
    {
        typedef lex::lexertl::token<Iterator, Attribute, HasState> token_type;

        template <typename Out>
        static void print(Out& out, token_type const& val) 
        {
            out << '<';
            spirit::traits::print_token(out, val.value());
            out << '>';
        }
    };

}}}

#endif
