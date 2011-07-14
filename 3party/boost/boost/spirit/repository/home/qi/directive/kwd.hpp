/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011 Thomas Bernard

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(SPIRIT_KWD_NOVEMBER_14_2008_1148AM)
#define SPIRIT_KWD_NOVEMBER_14_2008_1148AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/auxiliary/lazy.hpp>
#include <boost/spirit/home/qi/operator/kleene.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/qi/detail/fail_function.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/repository/home/support/kwd.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/foreach.hpp>
#include <vector>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
         
    template < typename T>
    struct use_directive<qi::domain
      , terminal_ex<repository::tag::kwd                     // enables kwd(key)[p]
        , fusion::vector1<T > >
    > : traits::is_string<T> {};
    
    template < typename T>
    struct use_directive<qi::domain
      , terminal_ex<repository::tag::ikwd                     // enables ikwd(key)[p]
        , fusion::vector1<T > >
    > : traits::is_string<T> {};
    
    
    template < typename T1, typename T2>
    struct use_directive<qi::domain
      , terminal_ex<repository::tag::kwd                     // enables kwd(key,exact)[p]
        , fusion::vector2< T1, T2 > >
    > : traits::is_string<T1> {};

    template < typename T1, typename T2>
    struct use_directive<qi::domain
      , terminal_ex<repository::tag::ikwd                     // enables ikwd(key,exact)[p]
        , fusion::vector2< T1, T2 > >
    > : traits::is_string<T1> {};

    template < typename T1, typename T2>
    struct use_directive<qi::domain
      , terminal_ex<repository::tag::kwd                     // enables kwd(min, max)[p]
        , fusion::vector3< T1, T2, T2 > >
    > : traits::is_string<T1> {};

    template < typename T1, typename T2>
    struct use_directive<qi::domain
      , terminal_ex<repository::tag::ikwd                     // enables ikwd(min, max)[p]
        , fusion::vector3< T1, T2, T2 > >
    > : traits::is_string<T1> {};

    template < typename T1, typename T2>
    struct use_directive<qi::domain
      , terminal_ex<repository::tag::kwd                     // enables kwd(min, inf)[p]
        , fusion::vector3<T1, T2, inf_type > >
    > : traits::is_string<T1> {};
    
    template < typename T1, typename T2>
    struct use_directive<qi::domain
      , terminal_ex<repository::tag::ikwd                     // enables ikwd(min, inf)[p]
        , fusion::vector3<T1, T2, inf_type > >
    > : traits::is_string<T1> {};


  /*  template <>                                     // enables *lazy* kwd(exact)[p]
    struct use_lazy_directive<
        qi::domain
      , tag::kwd
      , 1 // arity
    > : mpl::true_ {};

    template <>                                     // enables *lazy* kwd(min, max)[p]
    struct use_lazy_directive<                      // and kwd(min, inf)[p]
        qi::domain
      , tag::kwd
      , 2 // arity
    > : mpl::true_ {};
*/

}}

namespace boost { namespace spirit { namespace repository { namespace qi
{
    using repository::kwd;
    using repository::ikwd;
    using spirit::inf;
    using spirit::inf_type;

template <typename T>
    struct kwd_pass_iterator // handles kwd(exact)[p]
    {
        kwd_pass_iterator() {}
        bool flag_init() const { return true; }
        bool register_successful_parse(bool &flag,T &i) const {
            flag=true;
            return true;
        }
        

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        kwd_pass_iterator& operator= (kwd_pass_iterator const&);
    };

    template <typename T>
    struct kwd_exact_iterator // handles kwd(exact)[p]
    {
        kwd_exact_iterator(T const exact)
          : exact(exact){}

        typedef T type;
        bool flag_init() const { return false; }
        bool register_successful_parse(bool &flag,T &i) const {
            i++;
            if(i<exact)
            {
                flag=false;
                return true;
            }
            else if(i==exact)
            {
                flag=true;
                return true;
            }
            else
                return flag=false;
            
        }
        T const exact;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        kwd_exact_iterator& operator= (kwd_exact_iterator const&);
    };

    template <typename T>
    struct kwd_finite_iterator // handles kwd(min, max)[p]
    {
        kwd_finite_iterator(T const min, T const max)
          : min BOOST_PREVENT_MACRO_SUBSTITUTION (min)
          , max BOOST_PREVENT_MACRO_SUBSTITUTION (max)
            {}

        typedef T type;
        bool flag_init() const { return min==0; }
        bool register_successful_parse(bool &flag,T &i) const {
            i++;
            if(i<min)
            {
                flag=false;
                return true;
            }
            else if(i>=min && i<=max)
            {
                return flag=true;
            }
            else
                return flag=false;
        }
        T const min;
        T const max;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        kwd_finite_iterator& operator= (kwd_finite_iterator const&);
    };

    template <typename T>
    struct kwd_infinite_iterator // handles kwd(min, inf)[p]
    {
        kwd_infinite_iterator(T const min)
          : min BOOST_PREVENT_MACRO_SUBSTITUTION (min) {}

        typedef T type;
        bool flag_init() const { return min==0; }
        bool register_successful_parse(bool &flag,T &i) const {
            i++;
            flag = i>=min;
            return true;
        }
        T const min;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        kwd_infinite_iterator& operator= (kwd_infinite_iterator const&);
    };

    // This class enables the transportation of parameters needed to call 
    // the occurence constraint checker from higher level calls
    // It also serves to select the correct parse function call 
    // of the keyword parser. The implementation changes depending if it is
    // called form a keyword parsing loop or not.
    template <typename Skipper, typename NoCasePass>
    struct skipper_keyword_marker
    {
        typedef NoCasePass no_case_pass;
        
        skipper_keyword_marker(Skipper const &skipper,bool &flag,int &counter) : 
              skipper(skipper)
            , flag(flag)
            , counter(counter) 
            {}
            
        const Skipper &skipper;
        bool &flag;
        int &counter;
    };
    
    template <typename Subject, typename KeywordType, typename LoopIter , typename NoCase >
    struct kwd_parser : spirit::qi::unary_parser<kwd_parser<Subject, KeywordType, LoopIter , NoCase > >
    {
        struct kwd_parser_id;
        
        typedef Subject subject_type;
        typedef NoCase no_case_keyword;
                
        typedef typename
            remove_const<typename traits::char_type_of<KeywordType>::type>::type
        char_type;
        
        typedef std::basic_string<char_type> keyword_type;
        
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename
            traits::build_std_vector<
                typename traits::attribute_of<
                Subject, Context, Iterator>::type
                        >::type
                type;
        };
        

        kwd_parser(Subject const& subject
           , typename add_reference<KeywordType>::type keyword
           , LoopIter const& iter)
          : subject(subject), iter(iter), keyword(keyword) {}    
        
        // Call the subject parser on a non container attribute
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse_impl(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr,mpl::false_) const
        {
            return subject.parse(first,last,context,skipper,attr);
        }
    
        // Call the subject parser on a container attribute
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse_impl(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr,mpl::true_) const
        {                   
            
            // synthesized attribute needs to be default constructed
            typename traits::container_value<Attribute>::type val =
                typename traits::container_value<Attribute>::type();

            Iterator save = first;
            bool r = subject.parse(first,last,context,skipper, val);
            if (r)
            {
                // push the parsed value into our attribute
                r = traits::push_back(attr, val);
                if (!r)
                    first = save;
            }
            return r;
        }
       
       template <typename Iterator, typename Context
          , typename Skipper, typename Attribute,typename NoCasePass>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, skipper_keyword_marker<Skipper,NoCasePass> const& skipper
          , Attribute &attr) const
        {
            
            typedef typename traits::attribute_of<
                Subject, Context, Iterator>::type
                subject_attribute;
            
            typedef typename mpl::and_<
             traits::is_container<Attribute>
            , mpl::not_< traits::is_weak_substitute< subject_attribute,Attribute > >
            >::type predicate;
            
            if((no_case_keyword::value && NoCasePass::value) || !NoCasePass::value)
            {
                if(parse_impl(first,last,context,skipper.skipper,attr, predicate()))
                    return iter.register_successful_parse(skipper.flag,skipper.counter);
            }
            return false;
        }
       
        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr) const
          {
              typedef typename traits::attribute_of<
                Subject, Context, Iterator>::type
                subject_attribute;
            
            typedef typename mpl::and_<
            traits::is_container<Attribute>
            , mpl::not_< traits::is_weak_substitute< subject_attribute,Attribute > >
            >::type predicate;
            
            // Parse the keyword
            bool flag = iter.flag_init();
            int counter = 0;
            Iterator save = first;
            spirit::qi::skip_over(first, last, skipper);
            if(keyword.parse(first,last,context,skipper,unused)){
                // Followed by the subject parser
                spirit::qi::skip_over(first, last, skipper);
                if(parse_impl(first,last,context,skipper,attr, predicate()))
                {
                    return iter.register_successful_parse(flag,counter);
                }
            }
            first = save;            
            return flag;            
          }
       
    
        template <typename Context>
        info what(Context& context) const
        {
            if(no_case_keyword::value)
                return info("ikwd", subject.what(context));
            else
                return info("kwd", subject.what(context));
        }

        Subject subject;
        LoopIter iter;            
        
        spirit::qi::literal_string<KeywordType, true> keyword;
    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        kwd_parser& operator= (kwd_parser const&);

        template <typename Iterator, typename Context, typename Skipper>
        static spirit::qi::detail::fail_function<Iterator, Context, Skipper>
        fail_function(
            Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper)
        {
            return spirit::qi::detail::fail_function<Iterator, Context, Skipper>
                (first, last, context, skipper);
        }


    };
}}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace qi
{

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    
     // Directive kwd(key)[p]
    template <typename T1,  typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<repository::tag::kwd, fusion::vector1<T1> >, Subject, Modifiers>
    {
        typedef typename add_const<T1>::type const_keyword;
        typedef repository::qi::kwd_pass_iterator<int> iterator_type;
        typedef has_modifier<Modifiers, tag::char_code_base<tag::no_case> > no_case;
        
                
        typedef repository::qi::kwd_parser<Subject, const_keyword, iterator_type, no_case > result_type;

        template <typename Terminal>
        result_type operator()(
            Terminal const& term, Subject const& subject, unused_type) const
        {
            return result_type(subject
                        ,fusion::at_c<0>(term.args)
                        ,iterator_type()
                        );
        }
    };
    
    // Directive ikwd(key)[p]
    template <typename T1,  typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<repository::tag::ikwd, fusion::vector1<T1> >, Subject, Modifiers>
    {
        typedef typename add_const<T1>::type const_keyword;
        typedef repository::qi::kwd_pass_iterator<int> iterator_type;
        
        typedef repository::qi::kwd_parser<Subject, const_keyword, iterator_type, mpl::true_ > result_type;

        template <typename Terminal>
        result_type operator()(
            Terminal const& term, Subject const& subject, unused_type) const
        {
       /*     typename spirit::detail::get_encoding<Modifiers,
                spirit::char_encoding::standard>::type encoding;*/
                
            return result_type(subject
                        ,fusion::at_c<0>(term.args)
                        ,iterator_type()                        
                        );
        }
    };
    
    // Directive kwd(key,exact)[p]
    template <typename T1, typename T2,  typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<repository::tag::kwd, fusion::vector2<T1,T2> >, Subject, Modifiers>
    {
        typedef typename add_const<T1>::type const_keyword;
        typedef repository::qi::kwd_exact_iterator<T2> iterator_type;
        typedef has_modifier<Modifiers, tag::char_code_base<tag::no_case> > no_case;
        
        typedef repository::qi::kwd_parser<Subject, const_keyword, iterator_type, no_case > result_type;

        template <typename Terminal>
        result_type operator()(
            Terminal const& term, Subject const& subject, unused_type) const
        {
            return result_type(subject
                        ,fusion::at_c<0>(term.args)
                        ,fusion::at_c<1>(term.args)
                        );
        }
    };

    // Directive ikwd(key,exact)[p]
    template <typename T1, typename T2,  typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<repository::tag::ikwd, fusion::vector2<T1,T2> >, Subject, Modifiers>
    {
        typedef typename add_const<T1>::type const_keyword;
        typedef repository::qi::kwd_exact_iterator<T2> iterator_type;
        
        typedef repository::qi::kwd_parser<Subject, const_keyword, iterator_type, mpl::true_ > result_type;
        
        template <typename Terminal>
        result_type operator()(
            Terminal const& term, Subject const& subject, unused_type) const
        {            
            return result_type(subject
                        , fusion::at_c<0>(term.args)
                        , fusion::at_c<1>(term.args)
                        );
        }
    };
    
    // Directive kwd(min, max)[p]
    template <typename T1, typename T2, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<repository::tag::kwd, fusion::vector3< T1, T2, T2> >, Subject, Modifiers>
    {
        typedef typename add_const<T1>::type const_keyword;
        typedef repository::qi::kwd_finite_iterator<T2> iterator_type;
        typedef has_modifier<Modifiers, tag::char_code_base<tag::no_case> > no_case;
                    
        typedef repository::qi::kwd_parser<Subject, const_keyword, iterator_type, no_case > result_type;

        template <typename Terminal>
        result_type operator()(
            Terminal const& term, Subject const& subject, unused_type) const
        {
            return result_type(subject, fusion::at_c<0>(term.args),
                iterator_type(
                    fusion::at_c<1>(term.args)
                  , fusion::at_c<2>(term.args)
                )
            );
        }
    };

    // Directive ikwd(min, max)[p]
    template <typename T1, typename T2, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<repository::tag::ikwd, fusion::vector3< T1, T2, T2> >, Subject, Modifiers>
    {
        typedef typename add_const<T1>::type const_keyword;
        typedef repository::qi::kwd_finite_iterator<T2> iterator_type;
                
        typedef repository::qi::kwd_parser<Subject, const_keyword, iterator_type, mpl::true_ > result_type;

        template <typename Terminal>
        result_type operator()(
            Terminal const& term, Subject const& subject, unused_type) const
        {
        
            return result_type(subject, fusion::at_c<0>(term.args),
                iterator_type(
                    fusion::at_c<1>(term.args)
                  , fusion::at_c<2>(term.args)
                )
            );
        }
    };
    
    // Directive kwd(min, inf)[p]
    template <typename T1, typename T2, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<repository::tag::kwd
        , fusion::vector3<T1, T2, inf_type> >, Subject, Modifiers>
    {
        typedef typename add_const<T1>::type const_keyword;
        typedef repository::qi::kwd_infinite_iterator<T2> iterator_type;
        typedef has_modifier<Modifiers, tag::char_code_base<tag::no_case> > no_case;
        
        typedef repository::qi::kwd_parser<Subject, const_keyword, iterator_type, no_case > result_type;

        template <typename Terminal>
        result_type operator()(
            Terminal const& term, Subject const& subject, unused_type) const
        {
            return result_type(subject
                , fusion::at_c<0>(term.args)
                , fusion::at_c<1>(term.args)
                );
        }
    };
    
    // Directive ikwd(min, inf)[p]
    template <typename T1, typename T2, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<repository::tag::ikwd
        , fusion::vector3<T1, T2, inf_type> >, Subject, Modifiers>
    {
        typedef typename add_const<T1>::type const_keyword;
        typedef repository::qi::kwd_infinite_iterator<T2> iterator_type;
        
        typedef repository::qi::kwd_parser<Subject, const_keyword, iterator_type, mpl::true_ > result_type;

        template <typename Terminal>
        result_type operator()(
            Terminal const& term, Subject const& subject, unused_type) const
        {
            typename spirit::detail::get_encoding<Modifiers,
                spirit::char_encoding::standard>::type encoding;
                
            return result_type(subject
                , fusion::at_c<0>(term.args)
                , fusion::at_c<1>(term.args)
                );
        }
    };
    
    
}}}

namespace boost { namespace spirit { namespace traits
{
    template <typename Subject, typename KeywordType
            , typename LoopIter, typename NoCase >
    struct has_semantic_action< 
            repository::qi::kwd_parser< Subject, KeywordType, LoopIter, NoCase > >
      : unary_has_semantic_action<Subject> {};
}}}

#endif

