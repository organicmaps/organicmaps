// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_BACK_DISPATCH_TABLE_H
#define BOOST_MSM_BACK_DISPATCH_TABLE_H

#include <utility>

#include <boost/mpl/reverse_fold.hpp>
#include <boost/mpl/greater.hpp>
#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/type_traits/is_base_of.hpp>

#include <boost/msm/back/metafunctions.hpp>
#include <boost/msm/back/common_types.hpp>

namespace boost { namespace msm { namespace back 
{

// Generates a singleton runtime lookup table that maps current state
// to a function that makes the SM take its transition on the given
// Event type.
template <class Fsm,class Stt, class Event,class CompilePolicy>
struct dispatch_table
{
 private:
    // This is a table of these function pointers.
    typedef HandledEnum (*cell)(Fsm&, int,int,Event const&);
    typedef bool (*guard)(Fsm&, Event const&);

    // class used to build a chain (or sequence) of transitions for a given event and start state
    // (like an UML diamond). Allows transition conflicts.
    template< typename Seq,typename AnEvent,typename State >
    struct chain_row
    {
        typedef State   current_state_type;
        typedef AnEvent transition_event;

        // helper for building a disable/enable_if-controlled execute function
        struct execute_helper
        {
            template <class Sequence>
            static
            HandledEnum
            execute(Fsm& , int, int, Event const& , ::boost::mpl::true_ const & )
            {
                // if at least one guard rejected, this will be ignored, otherwise will generate an error
                return HANDLED_FALSE;
            }

            template <class Sequence>
            static
            HandledEnum
            execute(Fsm& fsm, int region_index , int state, Event const& evt,
                    ::boost::mpl::false_ const & )
            {
                 // try the first guard
                 typedef typename ::boost::mpl::front<Sequence>::type first_row;
                 HandledEnum res = first_row::execute(fsm,region_index,state,evt);
                 if (HANDLED_TRUE!=res)
                 {
                     // if the first rejected, move on to the next one
                    HandledEnum sub_res = 
                         execute<typename ::boost::mpl::pop_front<Sequence>::type>(fsm,region_index,state,evt,
                            ::boost::mpl::bool_<
                                ::boost::mpl::empty<typename ::boost::mpl::pop_front<Sequence>::type>::type::value>());
                     // if at least one guards rejects, the event will not generate a call to no_transition
                     HandledEnum handled = ((HANDLED_GUARD_REJECT==sub_res) || 
                                            (HANDLED_GUARD_REJECT==res))?
                                                HANDLED_GUARD_REJECT:sub_res;
                     return handled;
                 }
                 return res;
            }
        };
        // Take the transition action and return the next state.
        static HandledEnum execute(Fsm& fsm, int region_index, int state, Event const& evt)
        {
            // forward to helper
            return execute_helper::template execute<Seq>(fsm,region_index,state,evt,
                ::boost::mpl::bool_< ::boost::mpl::empty<Seq>::type::value>());
        }
    };
    // nullary metafunction whose only job is to prevent early evaluation of _1
    template< typename Entry > 
    struct make_chain_row_from_map_entry
    { 
        typedef chain_row<typename Entry::second,Event,
            typename Entry::first > type;
    }; 
    // Compute the maximum state value in the sm so we know how big
    // to make the table
    typedef typename generate_state_set<Stt>::type state_list;
    BOOST_STATIC_CONSTANT(int, max_state = ( ::boost::mpl::size<state_list>::value));

    // A function object for use with mpl::for_each that stuffs
    // transitions into cells.
    struct init_cell
    {
        init_cell(dispatch_table* self_)
          : self(self_)
        {}
        // version for transition event not base of our event
        template <class Transition>
        void init_event_base_case(Transition const&, ::boost::mpl::true_ const &) const
        {
            typedef typename create_stt<Fsm>::type stt; 
            BOOST_STATIC_CONSTANT(int, state_id = 
                (get_state_id<stt,typename Transition::current_state_type>::value));
            self->entries[state_id] = reinterpret_cast<cell>(&Transition::execute);
        }
        // version for transition event base of our event
        template <class Transition>
        void init_event_base_case(Transition const&, ::boost::mpl::false_ const &) const
        {
            typedef typename create_stt<Fsm>::type stt; 
            BOOST_STATIC_CONSTANT(int, state_id = 
                (get_state_id<stt,typename Transition::current_state_type>::value));
            self->entries[state_id] = &Transition::execute;
        }
        // Cell initializer function object, used with mpl::for_each
        template <class Transition>
        typename ::boost::enable_if<typename has_not_real_row_tag<Transition>::type,void >::type
            operator()(Transition const&,boost::msm::back::dummy<0> = 0) const
        {
            // version for not real rows. No problem because irrelevant for process_event
        }
        template <class Transition>
        typename ::boost::disable_if<typename has_not_real_row_tag<Transition>::type,void >::type
        operator()(Transition const& tr,boost::msm::back::dummy<1> = 0) const
        {
            //only if the transition event is a base of our event is the reinterpret_case safe
            init_event_base_case(tr,
                ::boost::mpl::bool_< 
                    ::boost::is_base_of<typename Transition::transition_event,Event>::type::value>() );
        }
    
        dispatch_table* self;
    };

    // Cell default-initializer function object, used with mpl::for_each
    // initializes with call_no_transition, defer_transition or default_eventless_transition
    // variant for non-anonymous transitions
    template <class EventType,class Enable=void>
    struct default_init_cell
    {
        default_init_cell(dispatch_table* self_,cell* tofill_entries_)
            : self(self_),tofill_entries(tofill_entries_)
        {}
        template <class State>
        typename ::boost::enable_if<typename has_state_delayed_event<State,Event>::type,void>::type
        operator()(boost::msm::wrap<State> const&,boost::msm::back::dummy<0> = 0)
        {
            typedef typename create_stt<Fsm>::type stt; 
            BOOST_STATIC_CONSTANT(int, state_id = (get_state_id<stt,State>::value));
            cell call_no_transition = &Fsm::defer_transition;
            tofill_entries[state_id] = call_no_transition;
        }
        template <class State>
        typename ::boost::disable_if<typename has_state_delayed_event<State,Event>::type,void >::type
        operator()(boost::msm::wrap<State> const&,boost::msm::back::dummy<1> = 0)
        {
            typedef typename create_stt<Fsm>::type stt; 
            BOOST_STATIC_CONSTANT(int, state_id = (get_state_id<stt,State>::value));
            cell call_no_transition = &Fsm::call_no_transition;
            tofill_entries[state_id] = call_no_transition;
        }

        dispatch_table* self;
        cell* tofill_entries;
    };

    // variant for anonymous transitions
    template <class EventType>
    struct default_init_cell<EventType,
                             typename ::boost::enable_if<
                                typename is_completion_event<EventType>::type>::type>
    {
        default_init_cell(dispatch_table* self_,cell* tofill_entries_)
            : self(self_),tofill_entries(tofill_entries_)
        {}

        // this event is a compound one (not a real one, just one for use in event-less transitions)
        // Note this event cannot be used as deferred!
        template <class State>
        void operator()(boost::msm::wrap<State> const&)
        {
            typedef typename create_stt<Fsm>::type stt; 
            BOOST_STATIC_CONSTANT(int, state_id = (get_state_id<stt,State>::value));
            cell call_no_transition = &Fsm::default_eventless_transition;
            tofill_entries[state_id] = call_no_transition;
        }

        dispatch_table* self;
        cell* tofill_entries;
    };

 public:
    // initialize the dispatch table for a given Event and Fsm
    dispatch_table()
    {
        // Initialize cells for no transition
        ::boost::mpl::for_each<typename generate_state_set<Stt>::type, 
                               boost::msm::wrap< ::boost::mpl::placeholders::_1> >
                        (default_init_cell<Event>(this,entries));

        // build chaining rows for rows coming from the same state and the current event
        // first we build a map of sequence for every source
        // in reverse order so that the frow's are handled first (UML priority)
        typedef typename ::boost::mpl::reverse_fold<
                        // filter on event
                        ::boost::mpl::filter_view
                            <Stt, ::boost::is_base_of<transition_event< ::boost::mpl::placeholders::_>, Event> >,
                        // build a map
                        ::boost::mpl::map<>,
                        ::boost::mpl::if_<
                            // if we already have a row on this source state
                            ::boost::mpl::has_key< ::boost::mpl::placeholders::_1,
                                                   transition_source_type< ::boost::mpl::placeholders::_2> >,
                            // insert a new element in the value type
                            ::boost::mpl::insert< 
                                ::boost::mpl::placeholders::_1,
                                ::boost::mpl::pair<transition_source_type< ::boost::mpl::placeholders::_2>,
                                                   ::boost::mpl::push_back< 
                                                        ::boost::mpl::at< ::boost::mpl::placeholders::_1,
                                                        transition_source_type< ::boost::mpl::placeholders::_2> >,
                                                        ::boost::mpl::placeholders::_2 > 
                                                   > >,
                            // first row on this source state, make a vector with 1 element
                            ::boost::mpl::insert< 
                                        ::boost::mpl::placeholders::_1,
                                        ::boost::mpl::pair<transition_source_type< ::boost::mpl::placeholders::_2>,
                                        make_vector< ::boost::mpl::placeholders::_2> > >
                               >
                       >::type map_of_row_seq;
        // and then build chaining rows for all source states having more than 1 row
        typedef typename ::boost::mpl::fold<
            map_of_row_seq,::boost::mpl::vector0<>,
            ::boost::mpl::if_<
                     ::boost::mpl::greater< ::boost::mpl::size< 
                                                    ::boost::mpl::second< ::boost::mpl::placeholders::_2> >,
                                            ::boost::mpl::int_<1> >,
                     // we need row chaining
                     ::boost::mpl::push_back< ::boost::mpl::placeholders::_1, 
                                    make_chain_row_from_map_entry< ::boost::mpl::placeholders::_2> >,
                     // just one row, no chaining, we rebuild the row like it was before
                     ::boost::mpl::push_back< ::boost::mpl::placeholders::_1, 
                                              get_first_element_pair_second< ::boost::mpl::placeholders::_2> > 
             > >::type chained_rows; 
        // Go back and fill in cells for matching transitions.
        ::boost::mpl::for_each<chained_rows>(init_cell(this));
    }

    // The singleton instance.
    static const dispatch_table instance;

 public: // data members
    cell entries[max_state];
};

}}} // boost::msm::back


#endif //BOOST_MSM_BACK_DISPATCH_TABLE_H

