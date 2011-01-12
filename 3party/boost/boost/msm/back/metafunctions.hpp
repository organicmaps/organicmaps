// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_BACK_METAFUNCTIONS_H
#define BOOST_MSM_BACK_METAFUNCTIONS_H

#include <boost/mpl/set.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/count_if.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/insert_range.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/logical.hpp>

#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>

BOOST_MPL_HAS_XXX_TRAIT_DEF(explicit_creation)
BOOST_MPL_HAS_XXX_TRAIT_DEF(pseudo_entry)
BOOST_MPL_HAS_XXX_TRAIT_DEF(pseudo_exit)
BOOST_MPL_HAS_XXX_TRAIT_DEF(concrete_exit_state)
BOOST_MPL_HAS_XXX_TRAIT_DEF(composite_tag)
BOOST_MPL_HAS_XXX_TRAIT_DEF(not_real_row_tag)
BOOST_MPL_HAS_XXX_TRAIT_DEF(event_blocking_flag)
BOOST_MPL_HAS_XXX_TRAIT_DEF(explicit_entry_state)
BOOST_MPL_HAS_XXX_TRAIT_DEF(completion_event)
BOOST_MPL_HAS_XXX_TRAIT_DEF(no_exception_thrown)
BOOST_MPL_HAS_XXX_TRAIT_DEF(no_message_queue)
BOOST_MPL_HAS_XXX_TRAIT_DEF(activate_deferred_events)
BOOST_MPL_HAS_XXX_TRAIT_DEF(wrapped_entry)

namespace boost { namespace msm { namespace back
{

// returns the current state type of a transition
template <class Transition>
struct transition_source_type
{
    typedef typename Transition::current_state_type type;
};

// returns the target state type of a transition
template <class Transition>
struct transition_target_type
{
    typedef typename Transition::next_state_type type;
};

// helper functions for generate_state_ids
// create a pair of a state and a passed id for source and target states
template <class Id,class Transition>
struct make_pair_source_state_id
{
    typedef typename ::boost::mpl::pair<typename Transition::current_state_type,Id> type;
};
template <class Id,class Transition>
struct make_pair_target_state_id
{
    typedef typename ::boost::mpl::pair<typename Transition::next_state_type,Id> type;
};

// iterates through a transition table and automatically generates ids starting at 0
// first the source states, transition up to down
// then the target states, up to down
template <class stt>
struct generate_state_ids
{
    typedef typename 
        ::boost::mpl::fold<
        stt,::boost::mpl::pair< ::boost::mpl::map< >, ::boost::mpl::int_<0> >,
        ::boost::mpl::pair<
            ::boost::mpl::if_<
                     ::boost::mpl::has_key< ::boost::mpl::first< ::boost::mpl::placeholders::_1>,
                                            transition_source_type< ::boost::mpl::placeholders::_2> >,
                     ::boost::mpl::first< ::boost::mpl::placeholders::_1>,
                     ::boost::mpl::insert< ::boost::mpl::first<mpl::placeholders::_1>,
                                make_pair_source_state_id< ::boost::mpl::second< ::boost::mpl::placeholders::_1 >,
                                                           ::boost::mpl::placeholders::_2> >
                      >,
            ::boost::mpl::if_<
                    ::boost::mpl::has_key< ::boost::mpl::first< ::boost::mpl::placeholders::_1>,
                                           transition_source_type< ::boost::mpl::placeholders::_2> >,
                    ::boost::mpl::second< ::boost::mpl::placeholders::_1 >,
                    ::boost::mpl::next< ::boost::mpl::second<mpl::placeholders::_1 > >
                    >
        > //pair
        >::type source_state_ids;
    typedef typename ::boost::mpl::first<source_state_ids>::type source_state_map;
    typedef typename ::boost::mpl::second<source_state_ids>::type highest_state_id;


    typedef typename 
        ::boost::mpl::fold<
        stt,::boost::mpl::pair<source_state_map,highest_state_id >,
        ::boost::mpl::pair<
            ::boost::mpl::if_<
                     ::boost::mpl::has_key< ::boost::mpl::first< ::boost::mpl::placeholders::_1>,
                                            transition_target_type< ::boost::mpl::placeholders::_2> >,
                     ::boost::mpl::first< ::boost::mpl::placeholders::_1>,
                     ::boost::mpl::insert< ::boost::mpl::first< ::boost::mpl::placeholders::_1>,
                                make_pair_target_state_id< ::boost::mpl::second< ::boost::mpl::placeholders::_1 >,
                                ::boost::mpl::placeholders::_2> >
                     >,
            ::boost::mpl::if_<
                    ::boost::mpl::has_key< ::boost::mpl::first< ::boost::mpl::placeholders::_1>,
                                           transition_target_type< ::boost::mpl::placeholders::_2> >,
                    ::boost::mpl::second< ::boost::mpl::placeholders::_1 >,
                    ::boost::mpl::next< ::boost::mpl::second< ::boost::mpl::placeholders::_1 > >
                    >
        > //pair
        >::type all_state_ids;
    typedef typename ::boost::mpl::first<all_state_ids>::type type;
};

// returns the id of a given state
template <class stt,class State>
struct get_state_id
{
    typedef typename ::boost::mpl::at<typename generate_state_ids<stt>::type,State>::type type;
    enum {value = type::value};
};

// returns a mpl::vector containing the init states of a state machine
template <class States>
struct get_initial_states 
{
    typedef typename ::boost::mpl::if_<
        ::boost::mpl::is_sequence<States>,
        States,
        typename ::boost::mpl::push_back< ::boost::mpl::vector0<>,States>::type >::type type;
};

// returns a mpl::int_ containing the size of a region. If the argument is not a sequence, returns 1
template <class region>
struct get_number_of_regions 
{
    typedef typename mpl::if_<
        ::boost::mpl::is_sequence<region>,
        ::boost::mpl::size<region>,
        ::boost::mpl::int_<1> >::type type;
};

// builds a mpl::vector of initial states
template <class region>
struct get_regions_as_sequence 
{
    typedef typename ::boost::mpl::if_<
        ::boost::mpl::is_sequence<region>,
        region,
        typename ::boost::mpl::push_back< ::boost::mpl::vector0<>,region>::type >::type type;
};

template <class ToCreateSeq>
struct get_explicit_creation_as_sequence 
{
    typedef typename ::boost::mpl::if_<
        ::boost::mpl::is_sequence<ToCreateSeq>,
        ToCreateSeq,
        typename ::boost::mpl::push_back< ::boost::mpl::vector0<>,ToCreateSeq>::type >::type type;
};

// returns true if 2 transitions have the same source (used to remove duplicates in search of composite states)
template <class stt,class Transition1,class Transition2>
struct have_same_source
{
    enum {current_state1 = get_state_id<stt,typename Transition1::current_state_type >::type::value};
    enum {current_state2 = get_state_id<stt,typename Transition2::current_state_type >::type::value};
    enum {value = ((int)current_state1 == (int)current_state2) };
};


// A metafunction that returns the Event associated with a transition.
template <class Transition>
struct transition_event
{
    typedef typename Transition::transition_event type;
};

// returns true for composite states
template <class State>
struct is_composite_state
{
    enum {value = has_composite_tag<State>::type::value};
    typedef typename has_composite_tag<State>::type type;
};

// transform a transition table in a container of source states
template <class stt>
struct keep_source_names
{
    // instead of the rows we want only the names of the states (from source)
    typedef typename 
        ::boost::mpl::transform<
        stt,transition_source_type< ::boost::mpl::placeholders::_1> >::type type;
};

// transform a transition table in a container of target states
template <class stt>
struct keep_target_names
{
    // instead of the rows we want only the names of the states (from source)
    typedef typename 
        ::boost::mpl::transform<
        stt,transition_target_type< ::boost::mpl::placeholders::_1> >::type type;
};

template <class stt>
struct generate_state_set
{
    // keep in the original transition table only the source/target state types
    typedef typename keep_source_names<stt>::type sources;
    typedef typename keep_target_names<stt>::type targets;
    typedef typename 
        ::boost::mpl::fold<
        sources, ::boost::mpl::set<>,
        ::boost::mpl::insert< ::boost::mpl::placeholders::_1, ::boost::mpl::placeholders::_2>
        >::type source_set;
    typedef typename 
        ::boost::mpl::fold<
        targets,source_set,
        ::boost::mpl::insert< ::boost::mpl::placeholders::_1, ::boost::mpl::placeholders::_2>
        >::type type;
};

// iterates through the transition table and generate a mpl::set<> containing all the events
template <class stt>
struct generate_event_set
{
    typedef typename 
        ::boost::mpl::fold<
            stt, ::boost::mpl::set<>,
            ::boost::mpl::if_<
                ::boost::mpl::has_key< ::boost::mpl::placeholders::_1, 
                                       transition_event< ::boost::mpl::placeholders::_2> >,
                ::boost::mpl::placeholders::_1,
                ::boost::mpl::insert< ::boost::mpl::placeholders::_1,
                                      transition_event< ::boost::mpl::placeholders::_2> > >
        >::type type;
};

// returns a mpl::bool_<true> if State has Event as deferred event
template <class State, class Event>
struct has_state_delayed_event  
{
    typedef typename ::boost::mpl::find<typename State::deferred_events,Event>::type found;
    typedef typename ::boost::mpl::if_<
        ::boost::is_same<found,typename ::boost::mpl::end<typename State::deferred_events>::type >,
        ::boost::mpl::bool_<false>,
        ::boost::mpl::bool_<true> >::type type;
};
// returns a mpl::bool_<true> if State has any deferred event
template <class State>
struct has_state_delayed_events  
{
    typedef typename ::boost::mpl::if_<
        ::boost::mpl::empty<typename State::deferred_events>,
        ::boost::mpl::bool_<false>,
        ::boost::mpl::bool_<true> >::type type;
};

// Template used to create dummy entries for initial states not found in the stt.
template< typename T1 >
struct not_a_row
{
    typedef int not_real_row_tag;
    struct dummy_event 
    {
    };
    typedef T1                  current_state_type;
    typedef T1                  next_state_type;
    typedef dummy_event         transition_event;
};

// metafunctions used to find out if a state is entry, exit or something else
template <class State>
struct is_pseudo_entry 
{
    typedef typename ::boost::mpl::if_< typename has_pseudo_entry<State>::type,
        ::boost::mpl::bool_<true>,::boost::mpl::bool_<false> 
    >::type type;
};
// says if a state is an exit pseudo state
template <class State>
struct is_pseudo_exit 
{
    typedef typename ::boost::mpl::if_< typename has_pseudo_exit<State>::type,
        ::boost::mpl::bool_<true>, ::boost::mpl::bool_<false> 
    >::type type;
};
// says if a state is an exit pseudo state
template <class State>
struct is_direct_entry 
{
    typedef typename ::boost::mpl::if_< typename has_explicit_entry_state<State>::type,
        ::boost::mpl::bool_<true>, ::boost::mpl::bool_<false> 
    >::type type;
};

//converts a "fake" (simulated in a state_machine_ description )state into one which will really get created
template <class StateType,class CompositeType>
struct convert_fake_state
{
    // converts a state (explicit entry) into the state we really are going to create (explicit<>)
    typedef typename ::boost::mpl::if_<
        typename is_direct_entry<StateType>::type,
        typename CompositeType::template direct<StateType>,
        typename ::boost::mpl::identity<StateType>::type
    >::type type;
};

template <class StateType>
struct get_explicit_creation 
{
    typedef typename StateType::explicit_creation type;
};

template <class StateType>
struct get_wrapped_entry 
{
    typedef typename StateType::wrapped_entry type;
};
// used for states created with explicit_creation
// if the state is an explicit entry, we reach for the wrapped state
// otherwise, this returns the state itself
template <class StateType>
struct get_wrapped_state 
{
    typedef typename ::boost::mpl::eval_if<
                typename has_wrapped_entry<StateType>::type,
                get_wrapped_entry<StateType>,
                ::boost::mpl::identity<StateType> >::type type;
};

template <class Derived>
struct create_stt 
{
    //typedef typename Derived::transition_table stt;
    typedef typename Derived::real_transition_table Stt;
    // get the state set
    typedef typename generate_state_set<Stt>::type states;
    // transform the initial region(s) in a sequence
    typedef typename get_regions_as_sequence<typename Derived::initial_state>::type init_states;
    // iterate through the initial states and add them in the stt if not already there
    typedef typename 
        ::boost::mpl::fold<
        init_states,Stt,
        ::boost::mpl::if_<
                 ::boost::mpl::has_key<states, ::boost::mpl::placeholders::_2>,
                 ::boost::mpl::placeholders::_1,
                 ::boost::mpl::insert< ::boost::mpl::placeholders::_1, ::boost::mpl::end<mpl::placeholders::_1>,
                             not_a_row< get_wrapped_state< ::boost::mpl::placeholders::_2> > > 
                  >
        >::type with_init;
    // do the same for states marked as explicitly created
    typedef typename get_explicit_creation_as_sequence<
       typename ::boost::mpl::eval_if<
            typename has_explicit_creation<Derived>::type,
            get_explicit_creation<Derived>,
            ::boost::mpl::vector0<> >::type
        >::type fake_explicit_created;

    typedef typename 
        ::boost::mpl::transform<
        fake_explicit_created,convert_fake_state< ::boost::mpl::placeholders::_1,Derived> >::type explicit_created;

    typedef typename 
        ::boost::mpl::fold<
        explicit_created,with_init,
        ::boost::mpl::if_<
                 ::boost::mpl::has_key<states, ::boost::mpl::placeholders::_2>,
                 ::boost::mpl::placeholders::_1,
                 ::boost::mpl::insert< ::boost::mpl::placeholders::_1, ::boost::mpl::end<mpl::placeholders::_1>,
                             not_a_row< get_wrapped_state< ::boost::mpl::placeholders::_2> > > 
                  >
        >::type type;
};

// returns the transition table of a Composite state
template <class Composite>
struct get_transition_table
{
    typedef typename create_stt<Composite>::type type;
};
// recursively get a transition table for a given composite state.
// returns the transition table for this state + the tables of all composite sub states recursively
template <class Composite>
struct recursive_get_transition_table
{
    // get the transition table of the state if it's a state machine
    typedef typename ::boost::mpl::eval_if<typename is_composite_state<Composite>::type,
        get_transition_table<Composite>,
        ::boost::mpl::vector0<> >::type org_table;

    typedef typename generate_state_set<org_table>::type states;

    // and for every substate, recursively get the transition table if it's a state machine
    typedef typename ::boost::mpl::fold<
        states,org_table,
        ::boost::mpl::insert_range< ::boost::mpl::placeholders::_1, ::boost::mpl::end<mpl::placeholders::_1>,
        recursive_get_transition_table< ::boost::mpl::placeholders::_2 > >
    >::type type;

};

// metafunction used to say if a SM has pseudo exit states
template <class Derived>
struct has_fsm_deferred_events 
{
    typedef typename create_stt<Derived>::type Stt;
    typedef typename generate_state_set<Stt>::type state_list;

    typedef typename ::boost::mpl::or_<
        typename has_activate_deferred_events<Derived>::type,
        ::boost::mpl::bool_< ::boost::mpl::count_if<
                typename Derived::configuration,
                has_activate_deferred_events< ::boost::mpl::placeholders::_1 > >::value != 0> 
    >::type found_in_fsm;

    typedef typename ::boost::mpl::or_<
            found_in_fsm,
            ::boost::mpl::bool_< ::boost::mpl::count_if<
                state_list,has_state_delayed_events<
                    ::boost::mpl::placeholders::_1 > >::value != 0>
            >::type type;
};

// returns a mpl::bool_<true> if State has any delayed event
template <class Event>
struct is_completion_event  
{
    typedef typename ::boost::mpl::if_<
        has_completion_event<Event>,
        ::boost::mpl::bool_<true>,
        ::boost::mpl::bool_<false> >::type type;
};
// metafunction used to say if a SM has eventless transitions
template <class Derived>
struct has_fsm_eventless_transition 
{
    typedef typename create_stt<Derived>::type Stt;
    typedef typename generate_event_set<Stt>::type event_list;

    typedef ::boost::mpl::bool_< ::boost::mpl::count_if<
        event_list,is_completion_event< ::boost::mpl::placeholders::_1 > >::value != 0> type;
};
template <class Derived>
struct find_completion_events 
{
    typedef typename create_stt<Derived>::type Stt;
    typedef typename generate_event_set<Stt>::type event_list;

    typedef typename ::boost::mpl::fold<
        event_list, ::boost::mpl::set<>,
        ::boost::mpl::if_<
                 is_completion_event< ::boost::mpl::placeholders::_2>,
                 ::boost::mpl::insert< ::boost::mpl::placeholders::_1, ::boost::mpl::placeholders::_2 >, 
                 ::boost::mpl::placeholders::_1 >
    >::type type;
};

template <class Transition>
struct make_vector 
{
    typedef ::boost::mpl::vector<Transition> type;
};
template< typename Entry > 
struct get_first_element_pair_second
{ 
    typedef typename ::boost::mpl::front<typename Entry::second>::type type;
}; 

 //returns the owner of an explicit_entry state
 //which is the containing SM if the transition originates from outside the containing SM
 //or else the explicit_entry state itself
template <class State,class ContainingSM>
struct get_owner 
{
    typedef typename ::boost::mpl::if_<
        typename ::boost::mpl::not_<typename ::boost::is_same<typename State::owner,
                                                              ContainingSM >::type>::type,
        typename State::owner, 
        State >::type type;
};

template <class Sequence,class ContainingSM>
struct get_fork_owner 
{
    typedef typename ::boost::mpl::front<Sequence>::type seq_front;
    typedef typename ::boost::mpl::if_<
                    typename ::boost::mpl::not_<
                        typename ::boost::is_same<typename seq_front::owner,ContainingSM>::type>::type,
                    typename seq_front::owner, 
                    seq_front >::type type;
};

template <class StateType,class ContainingSM>
struct make_exit 
{
    typedef typename ::boost::mpl::if_<
             typename is_pseudo_exit<StateType>::type ,
             typename ContainingSM::template exit_pt<StateType>,
             typename ::boost::mpl::identity<StateType>::type
            >::type type;
};

template <class StateType,class ContainingSM>
struct make_entry 
{
    typedef typename ::boost::mpl::if_<
        typename is_pseudo_entry<StateType>::type ,
        typename ContainingSM::template entry_pt<StateType>,
        typename ::boost::mpl::if_<
                typename is_direct_entry<StateType>::type,
                typename ContainingSM::template direct<StateType>,
                typename ::boost::mpl::identity<StateType>::type
                >::type
        >::type type;
};
// metafunction used to say if a SM has pseudo exit states
template <class StateType>
struct has_exit_pseudo_states_helper 
{
    typedef typename StateType::stt Stt;
    typedef typename generate_state_set<Stt>::type state_list;

    typedef ::boost::mpl::bool_< ::boost::mpl::count_if<
                state_list,is_pseudo_exit< ::boost::mpl::placeholders::_1> >::value != 0> type;
};
template <class StateType>
struct has_exit_pseudo_states 
{
    typedef typename ::boost::mpl::eval_if<typename is_composite_state<StateType>::type,
        has_exit_pseudo_states_helper<StateType>,
        ::boost::mpl::bool_<false> >::type type;
};

template <class StateType>
struct is_state_blocking 
{
    typedef typename ::boost::mpl::fold<
        typename StateType::flag_list, ::boost::mpl::set<>,
        ::boost::mpl::if_<
                 has_event_blocking_flag< ::boost::mpl::placeholders::_2>,
                 ::boost::mpl::insert< ::boost::mpl::placeholders::_1, ::boost::mpl::placeholders::_2 >, 
                 ::boost::mpl::placeholders::_1 >
    >::type blocking_flags;

    typedef typename ::boost::mpl::if_<
        ::boost::mpl::empty<blocking_flags>,
        ::boost::mpl::bool_<false>,
        ::boost::mpl::bool_<true> >::type type;
};
// returns a mpl::bool_<true> if fsm has an event blocking flag in one of its substates
template <class StateType>
struct has_fsm_blocking_states  
{
    typedef typename create_stt<StateType>::type Stt;
    typedef typename generate_state_set<Stt>::type state_list;

    typedef typename ::boost::mpl::fold<
        state_list, ::boost::mpl::set<>,
        ::boost::mpl::if_<
                 is_state_blocking< ::boost::mpl::placeholders::_2>,
                 ::boost::mpl::insert< ::boost::mpl::placeholders::_1, ::boost::mpl::placeholders::_2 >, 
                 ::boost::mpl::placeholders::_1 >
    >::type blocking_states;

    typedef typename ::boost::mpl::if_<
        ::boost::mpl::empty<blocking_states>,
        ::boost::mpl::bool_<false>,
        ::boost::mpl::bool_<true> >::type type;
};

template <class StateType>
struct is_no_exception_thrown
{
    typedef ::boost::mpl::bool_< ::boost::mpl::count_if<
        typename StateType::configuration,
        has_no_exception_thrown< ::boost::mpl::placeholders::_1 > >::value != 0> found;

    typedef typename ::boost::mpl::or_<
        typename has_no_exception_thrown<StateType>::type,
        found
    >::type type;
};

template <class StateType>
struct is_no_message_queue
{
    typedef ::boost::mpl::bool_< ::boost::mpl::count_if<
        typename StateType::configuration,
        has_no_message_queue< ::boost::mpl::placeholders::_1 > >::value != 0> found;

    typedef typename ::boost::mpl::or_<
        typename has_no_message_queue<StateType>::type,
        found
    >::type type;
};

template <class StateType>
struct get_initial_event 
{
    typedef typename StateType::initial_event type;
};
// helper to find out if a SM has an active exit state and is therefore waiting for exiting
template <class StateType,class OwnerFct,class FSM>
inline
typename ::boost::enable_if<typename ::boost::mpl::and_<typename is_composite_state<FSM>::type,
                                                        typename is_pseudo_exit<StateType>::type>,bool >::type
is_exit_state_active(FSM& fsm)
{
    typedef typename OwnerFct::type Composite;
    //typedef typename create_stt<Composite>::type stt;
    typedef typename Composite::stt stt;
    int state_id = get_state_id<stt,StateType>::type::value;
    Composite& comp = fsm.template get_state<Composite&>();
    return (std::find(comp.current_state(),comp.current_state()+Composite::nr_regions::value,state_id)
                            !=comp.current_state()+Composite::nr_regions::value);
}
template <class StateType,class OwnerFct,class FSM>
inline
typename ::boost::disable_if<typename ::boost::mpl::and_<typename is_composite_state<FSM>::type,
                                                         typename is_pseudo_exit<StateType>::type>,bool >::type
is_exit_state_active(FSM&)
{
    return false;
}

} } }//boost::msm::back

#endif // BOOST_MSM_BACK_METAFUNCTIONS_H

