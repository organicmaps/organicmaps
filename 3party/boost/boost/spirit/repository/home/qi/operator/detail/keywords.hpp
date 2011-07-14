/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011 Thomas Bernard

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(SPIRIT_KEYWORDS_DETAIL_MARCH_13_2007_1145PM)
#define SPIRIT_KEYWORDS_DETAIL_MARCH_13_2007_1145PM

#if defined(_MSC_VER)
#pragma once
#endif

namespace boost { namespace spirit { namespace repository { namespace qi { namespace detail {
        
        // This helper class enables jumping over intermediate directives 
        // down the kwd parser iteration count checking policy
        struct register_successful_parse
        {
            template <typename Subject>
            static bool call(Subject const &subject,bool &flag, int &counter)
            {
                return subject.iter.register_successful_parse(flag,counter);
            }
            template <typename Subject, typename Action>
            static bool call(spirit::qi::action<Subject, Action> const &subject,bool &flag, int &counter)
            {
                return subject.subject.iter.register_successful_parse(flag,counter);
            }
            template <typename Subject>
            static bool call(spirit::qi::hold_directive<Subject> const &subject,bool &flag, int &counter)
            {
                return subject.subject.iter.register_successful_parse(flag,counter);
            }
        };
        // Variant visitor class which handles dispatching the parsing to the selected parser
        // This also handles passing the correct attributes and flags/counters to the subject parsers       
 
        template < typename Elements, typename Iterator ,typename Context ,typename Skipper
                  ,typename Flags ,typename Counters ,typename Attribute, typename NoCasePass>
        class parse_dispatcher
            : public boost::static_visitor<bool>
        {
            public:
            parse_dispatcher(const Elements &elements,Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Flags &flags, Counters &counters, Attribute& attr) : 
                 elements(elements), first(first), last(last)
               , context(context), skipper(skipper)
               , flags(flags),counters(counters), attr(attr)
            {}
            
            template<typename T> bool operator()(T& idx) const
            {    
                return call(idx,typename traits::not_is_unused<Attribute>::type());
            }
            
            template <typename Subject,typename Index> 
            bool call_subject_unused(
                  Subject const &subject, Iterator &first, Iterator const &last
                , Context& context, Skipper const& skipper
                , Index& idx ) const
            {
                Iterator save = first;
                skipper_keyword_marker<Skipper,NoCasePass> marked_skipper(skipper,flags[Index::value],counters[Index::value]);
                
                if(subject.parse(first,last,context,marked_skipper,unused))
                {
                        return true;
                }
                save = save;
                return false;
            }            
 
            
            template <typename Subject,typename Index> 
            bool call_subject(
                  Subject const &subject, Iterator &first, Iterator const &last
                , Context& context, Skipper const& skipper
                , Index& idx ) const
            {
               
                Iterator save = first;
                skipper_keyword_marker<Skipper,NoCasePass> marked_skipper(skipper,flags[Index::value],counters[Index::value]);
                if(subject.parse(first,last,context,marked_skipper,fusion::at_c<Index::value>(attr)))
                {
                        return true;
                }
                save = save;
                return false;
            }

            // Handle unused attributes
            template <typename T> bool call(T &idx, mpl::false_) const{                            
                return call_subject_unused(fusion::at_c<T::value>(elements), first, last, context, skipper, idx );
            }
            // Handle normal attributes
            template <typename T> bool call(T &idx, mpl::true_) const{
                return call_subject(fusion::at_c<T::value>(elements), first, last, context, skipper, idx);
            }
            
            const Elements &elements;
            Iterator &first;
            const Iterator &last;
            Context & context;
            const Skipper &skipper;
            Flags &flags;
            Counters &counters;
            Attribute &attr;
        };
      
}}}}}

#endif
