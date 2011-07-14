/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011 Thomas Bernard

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(SPIRIT_KEYWORDS_OR_MARCH_13_2007_1145PM)
#define SPIRIT_KEYWORDS_OR_MARCH_13_2007_1145PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/detail/permute_function.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/detail/what_function.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/fusion/include/iter_fold.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/value_at.hpp>
#include <boost/optional.hpp>
#include <boost/foreach.hpp>
#include <boost/array.hpp>
#include <boost/spirit/home/qi/string/symbols.hpp>
#include <boost/spirit/home/qi/string/lit.hpp>
#include <boost/spirit/home/qi/action/action.hpp>
#include <boost/spirit/home/qi/directive/hold.hpp>
#include <boost/mpl/count_if.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/spirit/repository/home/qi/operator/detail/keywords.hpp>


namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<qi::domain, proto::tag::divides > // enables /
      : mpl::true_ {};

    template <>
    struct flatten_tree<qi::domain, proto::tag::divides> // flattens /
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace repository { namespace qi
{

    // kwd directive parser type identification
    namespace detail
    {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(kwd_parser_id)        

    
    }
                
    // kwd directive type query
    template <typename T>
    struct is_kwd_parser : detail::has_kwd_parser_id<T> {};

    template <typename Subject, typename Action>
    struct is_kwd_parser<spirit::qi::action<Subject,Action> > : detail::has_kwd_parser_id<Subject> {};

    template <typename Subject>
    struct is_kwd_parser<spirit::qi::hold_directive<Subject> > : detail::has_kwd_parser_id<Subject> {};

    // Keywords operator
    template <typename Elements, typename Modifiers>
    struct keywords : spirit::qi::nary_parser<keywords<Elements,Modifiers> >
    {
        template <typename Context, typename Iterator>
        struct attribute
        {
            // Put all the element attributes in a tuple
            typedef typename traits::build_attribute_sequence<
                Elements, Context, traits::sequence_attribute_transform, Iterator, spirit::qi::domain >::type
            all_attributes;

            // Now, build a fusion vector over the attributes. Note
            // that build_fusion_vector 1) removes all unused attributes
            // and 2) may return unused_type if all elements have
            // unused_type(s).
            typedef typename
                traits::build_fusion_vector<all_attributes>::type
            type;
        };
        
        /// Make sure that all subjects are of the kwd type
        typedef typename mpl::count_if< 
                Elements, 
                        mpl::not_< 
                            is_kwd_parser<
                                mpl::_1 
                            > 
                        >
                > non_kwd_subject_count;
        
        /// If the assertion fails here then you probably forgot to wrap a 
        /// subject of the / operator in a kwd directive
        BOOST_MPL_ASSERT_RELATION( non_kwd_subject_count::value, ==, 0 );
    
        ///////////////////////////////////////////////////////////////////////////
        // build_parser_tags
        //
        // Builds a boost::variant from an mpl::range_c in order to "mark" every 
        // parser of the fusion sequence. The integer constant is used in the parser
        // dispatcher functor in order to use the parser corresponding to the recognised
        // keyword.
        ///////////////////////////////////////////////////////////////////////////
        
        template <typename Sequence>
        struct build_parser_tags
        {
            // Get the sequence size
            typedef typename mpl::size< Sequence >::type sequence_size;

            // Create an integer_c constant for every parser in the sequence
            typedef typename mpl::range_c<int, 0, sequence_size::value>::type int_range;
            
            // Transform the range_c to an mpl vector in order to be able to transform it into a variant
            typedef typename mpl::copy<int_range, mpl::back_inserter<mpl::vector<> > >::type int_vector;
        
            // Build the variant type containing the indexes of the parsers
            typedef typename 
                spirit::detail::as_variant<
                    int_vector >::type        type;
        };
        
        // Create a variant type to be able to store parser indexes in the embedded symbols parser
        typedef typename build_parser_tags< Elements >::type parser_index_type;

        ///////////////////////////////////////////////////////////////////////////
        // build_char_type_sequence
        //
        // Build a fusion sequence from the kwd directive specified character type. 
        ///////////////////////////////////////////////////////////////////////////
        template <typename Sequence >
        struct build_char_type_sequence
        {
            struct element_char_type
            {
                template <typename T>
                struct result;

                template <typename F, typename Element>
                struct result<F(Element)>
                {
                    typedef typename Element::char_type type;
                        
                };
                template <typename F, typename Element,typename Action>
                struct result<F(spirit::qi::action<Element,Action>) >
                {
                    typedef typename Element::char_type type;
                };
                template <typename F, typename Element>
                struct result<F(spirit::qi::hold_directive<Element>)>
                {   
                    typedef typename Element::char_type type;
                };

                // never called, but needed for decltype-based result_of (C++0x)
                template <typename Element>
                typename result<element_char_type(Element)>::type
                operator()(Element&) const;
            };

            // Compute the list of character types of the child kwd directives
            typedef typename
                fusion::result_of::transform<Sequence, element_char_type>::type
            type;
        };
    
    
        ///////////////////////////////////////////////////////////////////////////
        // get_keyword_char_type
        //
        // Collapses the character type comming from the subject kwd parsers and
        // and checks that they are all identical (necessary in order to be able 
        // to build a tst parser to parse the keywords. 
        ///////////////////////////////////////////////////////////////////////////
        template <typename Sequence>
        struct get_keyword_char_type
        {
            // Make sure each of the types occur only once in the type list
            typedef typename
                mpl::fold<
                    Sequence, mpl::vector<>,
                    mpl::if_<
                        mpl::contains<mpl::_1, mpl::_2>,
                        mpl::_1, mpl::push_back<mpl::_1, mpl::_2>
                    >
                >::type
            no_duplicate_char_types;
        
            // If the compiler traps here this means you mixed 
            // character type for the keywords specified in the 
            // kwd directive sequence.
            BOOST_MPL_ASSERT_RELATION( mpl::size<no_duplicate_char_types>::value, ==, 1 );
            
            typedef typename mpl::front<no_duplicate_char_types>::type type;
                
        };

        /// Get the character type for the tst parser
        typedef typename build_char_type_sequence< Elements >::type char_types;
        typedef typename  get_keyword_char_type< char_types >::type  char_type;
        
        /// Our symbols container
        typedef spirit::qi::tst< char_type, parser_index_type> keywords_type;

        // Filter functor used for case insensitive parsing
        template <typename CharEncoding>
        struct no_case_filter
        {
            char_type operator()(char_type ch) const
            {
                return static_cast<char_type>(CharEncoding::tolower(ch));
            }
        };
    
        ///////////////////////////////////////////////////////////////////////////
        // build_case_type_sequence
        //
        // Build a fusion sequence from the kwd/ikwd directives 
        // in order to determine if case sensitive and case insensitive 
        // keywords have been mixed. 
        ///////////////////////////////////////////////////////////////////////////
        template <typename Sequence >
        struct build_case_type_sequence
        {
            struct element_case_type
            {
                template <typename T>
                struct result;

                template <typename F, typename Element>
                struct result<F(Element)>
                {
                    typedef typename Element::no_case_keyword type;
                        
                };
                template <typename F, typename Element,typename Action>
                struct result<F(spirit::qi::action<Element,Action>) >
                {
                    typedef typename Element::no_case_keyword type;
                };
                template <typename F, typename Element>
                struct result<F(spirit::qi::hold_directive<Element>)>
                {
                    typedef typename Element::no_case_keyword type;
                };

                // never called, but needed for decltype-based result_of (C++0x)
                template <typename Element>
                typename result<element_case_type(Element)>::type
                operator()(Element&) const;
            };

            // Compute the list of character types of the child kwd directives
            typedef typename
                fusion::result_of::transform<Sequence, element_case_type>::type
            type;
        };

        ///////////////////////////////////////////////////////////////////////////
        // get_nb_case_types
        //
        // Counts the number of entries in the case type sequence matching the 
        // CaseType parameter (mpl::true_  -> case insensitve
        //                   , mpl::false_ -> case sensitive
        ///////////////////////////////////////////////////////////////////////////
        template <typename Sequence,typename CaseType>
        struct get_nb_case_types
        {
            // Make sure each of the types occur only once in the type list
            typedef typename
                mpl::count_if<
                    Sequence, mpl::equal_to<mpl::_,CaseType> 
                >::type type;
                
            
        };
        // Build the case type sequence
        typedef typename build_case_type_sequence<Elements>::type case_type_sequence;
        // Count the number of case sensitive entries and case insensitve entries
        typedef typename get_nb_case_types<case_type_sequence,mpl::true_>::type ikwd_count;
        typedef typename get_nb_case_types<case_type_sequence,mpl::false_>::type kwd_count;
        // Get the size of the original sequence
        typedef typename mpl::size<Elements>::type nb_elements;
        // Determine if all the kwd directive are case sensitive/insensitive
        typedef typename mpl::equal_to< ikwd_count, nb_elements>::type all_ikwd;
        typedef typename mpl::equal_to< kwd_count, nb_elements>::type all_kwd;
        
        typedef typename mpl::or_< all_kwd, all_ikwd >::type all_directives_of_same_type;
             
        // Do we have a no case modifier
        typedef has_modifier<Modifiers, spirit::tag::char_code_base<spirit::tag::no_case> > no_case_modifier;

        // Should the no_case filter always be used ?
        typedef typename mpl::or_<
                            no_case_modifier,
                            mpl::and_< 
                                 all_directives_of_same_type
                                ,all_ikwd
                                >
                            >::type
                         no_case;
        
        typedef no_case_filter<
            typename spirit::detail::get_encoding_with_case<
                Modifiers
              , char_encoding::standard
              , no_case::value>::type>
        nc_filter;
        // Determine the standard case filter type
        typedef typename mpl::if_<
            no_case
          , nc_filter
          , spirit::qi::tst_pass_through >::type
        filter_type;
        
        
        // build a bool array and an integer array which will be used to 
        // check that the repetition constraints of the kwd parsers are 
        // met and bail out a soon as possible
        typedef boost::array<bool, fusion::result_of::size<Elements>::value> flags_type;
        typedef boost::array<int, fusion::result_of::size<Elements>::value> counters_type;
        
        

        // Functor which adds all the keywords/subject parser indexes 
        // collected from the subject kwd directives to the keyword tst parser       
        template< typename Sequence >
        struct keyword_entry_adder
        {
            typedef int result_type;

            keyword_entry_adder(shared_ptr<keywords_type> lookup,flags_type &flags) :
                lookup(lookup)
                ,flags(flags)
                {}
            
            typedef typename fusion::result_of::begin< Sequence >::type sequence_begin;
            
            template <typename T>
                int operator()(const int i, const T &parser) const
                {
                     // Determine the current position being handled
                    typedef typename fusion::result_of::distance< sequence_begin, T >::type position_raw;
                    // Transform the position to a parser index tag
                    typedef typename mpl::integral_c<int,position_raw::value> position;

                    return call(i,fusion::deref(parser),position());
                }

            template <typename T, typename Position, typename Action>
                int call( const int i, const spirit::qi::action<T,Action> &parser, const Position position ) const
                {
                    
                    // Make the keyword/parse index entry in the tst parser
                    lookup->add(
                        traits::get_begin<char_type>(parser.subject.keyword.str),
                        traits::get_end<char_type>(parser.subject.keyword.str), 
                        position
                        );
                    // Get the initial state of the flags array and store it in the flags initializer
                    flags[Position::value]=parser.subject.iter.flag_init();
                    return 0;
                }
    
            template <typename T, typename Position>
                int call( const int i, const T & parser, const Position position) const
                {
                    // Make the keyword/parse index entry in the tst parser
                    lookup->add(
                        traits::get_begin<char_type>(parser.keyword.str),
                        traits::get_end<char_type>(parser.keyword.str), 
                        position
                        );
                    // Get the initial state of the flags array and store it in the flags initializer
                    flags[Position::value]=parser.iter.flag_init();
                    return 0;
                }
       
            template <typename T, typename Position>
                int call( const int i, const spirit::qi::hold_directive<T> & parser, const Position position) const
                {
                    // Make the keyword/parse index entry in the tst parser
                    lookup->add(
                        traits::get_begin<char_type>(parser.subject.keyword.str),
                        traits::get_end<char_type>(parser.subject.keyword.str), 
                        position
                        );
                    // Get the initial state of the flags array and store it in the flags initializer
                    flags[Position::value]=parser.subject.iter.flag_init();
                    return 0;
                }
 

            shared_ptr<keywords_type> lookup;
            flags_type & flags;
        };
        
        
        keywords(Elements const& elements) :
              elements(elements)
            , lookup(new keywords_type())
        {
            // Loop through all the subject parsers to build the keyword parser symbol parser
            keyword_entry_adder<Elements> f1(lookup,flags_init);
            fusion::iter_fold(this->elements,0,f1);
        }
        
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_) const
        {
            // Select which parse function to call 
            // We need to handle the case where kwd / ikwd directives have been mixed
            // This is where we decide which function should be called.
            return parse_impl(first, last, context, skipper, attr_,
                                typename mpl::or_<all_directives_of_same_type, no_case>::type()
                             );
        }
        
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse_impl(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_,mpl::true_ /* no ikwd */) const
          {
           
            // wrap the attribute in a tuple if it is not a tuple
            typename traits::wrap_if_not_tuple<Attribute>::type attr(attr_);

            flags_type flags(flags_init);
            //flags.assign(false);
            
            counters_type counters;
            counters.assign(0);
                    
            typedef repository::qi::detail::parse_dispatcher<Elements,Iterator, Context, Skipper
                                    , flags_type, counters_type
                                    , typename traits::wrap_if_not_tuple<Attribute>::type
                                    , mpl::false_ > parser_visitor_type;
            parser_visitor_type parse_visitor(elements, first, last
                                             , context, skipper, flags
                                             , counters, attr);
            
            // We have a bool array 'flags' with one flag for each parser as well as a 'counter'
            // array.
            // The kwd directive sets and increments the counter when a successeful parse occured
            // as well as the slot of the corresponding parser to true in the flags array as soon 
            // the minimum repetition requirement is met and keeps that value to true as long as 
            // the maximum repetition requirement is met. 
            // The parsing takes place here in two steps:
            // 1) parse a keyword and fetch the parser index associated with that keyword
            // 2) call the associated parser and store the parsed value in the matching attribute.
            Iterator save = first;
            while(true)
            {
                
                spirit::qi::skip_over(first, last, skipper);
                if (parser_index_type* val_ptr
                    = lookup->find(first, last, filter_type()))
                {
                    spirit::qi::skip_over(first, last, skipper);
                    if(!apply_visitor(parse_visitor,*val_ptr)){
                        first = save;
                        return false;
                    }                
                    save = first;
                }
                else
                {
                    // Check that we are leaving the keywords parser in a successfull state
                    BOOST_FOREACH(bool &valid,flags)
                    {
                        if(!valid)
                        {
                            first = save;
                            return false;
                        }                        
                    }
                    return true;
                }
            }            
            return false;
        }
        
        // Handle the mixed kwd and ikwd case
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse_impl(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_,mpl::false_) const
          {
           
            // wrap the attribute in a tuple if it is not a tuple
            typename traits::wrap_if_not_tuple<Attribute>::type attr(attr_);

            flags_type flags(flags_init);
            //flags.assign(false);
            
            counters_type counters;
            counters.assign(0);
                    
            typedef detail::parse_dispatcher<Elements, Iterator, Context, Skipper
                                    , flags_type, counters_type
                                    , typename traits::wrap_if_not_tuple<Attribute>::type 
                                    , mpl::false_> parser_visitor_type;
            
           typedef detail::parse_dispatcher<Elements, Iterator, Context, Skipper
                                    , flags_type, counters_type
                                    , typename traits::wrap_if_not_tuple<Attribute>::type 
                                    , mpl::true_> no_case_parser_visitor_type;
            

            parser_visitor_type parse_visitor(elements,first,last
                                             ,context,skipper,flags,counters,attr);
            no_case_parser_visitor_type no_case_parse_visitor(elements,first,last
                                             ,context,skipper,flags,counters,attr);                    
            
            // We have a bool array 'flags' with one flag for each parser as well as a 'counter'
            // array.
            // The kwd directive sets and increments the counter when a successeful parse occured
            // as well as the slot of the corresponding parser to true in the flags array as soon 
            // the minimum repetition requirement is met and keeps that value to true as long as 
            // the maximum repetition requirement is met. 
            // The parsing takes place here in two steps:
            // 1) parse a keyword and fetch the parser index associated with that keyword
            // 2) call the associated parser and store the parsed value in the matching attribute.
            Iterator save = first;
            while(true)
            {
                spirit::qi::skip_over(first, last, skipper);
                // First pass case sensitive
                Iterator saved_first = first;
                if (parser_index_type* val_ptr
                    = lookup->find(first, last, spirit::qi::tst_pass_through()))
                {
                    spirit::qi::skip_over(first, last, skipper);
                    if(!apply_visitor(parse_visitor,*val_ptr)){
                        first = save;
                        return false;
                    }
                    save = first;
                }
                // Second pass case insensitive
                else if(parser_index_type* val_ptr
                    = lookup->find(saved_first,last,nc_filter()))
                {                    
                        first = saved_first;
                        spirit::qi::skip_over(first, last, skipper);
                        if(!apply_visitor(no_case_parse_visitor,*val_ptr)){
                            first = save;
                            return false;
                        }                
                        save = first;
                }
                else
                {
                    // Check that we are leaving the keywords parser in a successfull state
                    BOOST_FOREACH(bool &valid,flags)
                    {
                        if(!valid)
                        {
                            first = save;
                            return false;                        
                        }
                    }
                    return true;
                }
            }
            return false;
        }

        template <typename Context>
        info what(Context& context) const
        {
            info result("keywords");
            fusion::for_each(elements,
                spirit::detail::what_function<Context>(result, context));
            return result;
        }
        flags_type flags_init;
        Elements elements;
        shared_ptr<keywords_type> lookup;
        
    };
}}}}

namespace boost { namespace spirit { namespace qi {
    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements, typename Modifiers >
    struct make_composite<proto::tag::divides, Elements, Modifiers >
    {
        typedef repository::qi::keywords<Elements,Modifiers> result_type;
        result_type operator()(Elements ref, unused_type) const
        {
            return result_type(ref);
        }
    };
    
    
}}}

namespace boost { namespace spirit { namespace traits
{
    // We specialize this for keywords (see support/attributes.hpp).
    // For keywords, we only wrap the attribute in a tuple IFF
    // it is not already a fusion tuple.
    template <typename Elements, typename Modifiers,typename Attribute>
    struct pass_attribute<repository::qi::keywords<Elements,Modifiers>, Attribute>
      : wrap_if_not_tuple<Attribute> {};

    template <typename Elements, typename Modifiers>
    struct has_semantic_action<repository::qi::keywords<Elements, Modifiers> >
      : nary_has_semantic_action<Elements> {};
}}}

#endif

