// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_FRONT_FUNCTOR_ROW_H
#define BOOST_MSM_FRONT_FUNCTOR_ROW_H

#include <boost/mpl/set.hpp>
#include <boost/mpl/for_each.hpp>

#include <boost/typeof/typeof.hpp>

#include <boost/msm/row_tags.hpp>
#include <boost/msm/common.hpp>
#include <boost/msm/front/completion_event.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

namespace boost { namespace msm { namespace front
{
    template <class SOURCE,class EVENT,class TARGET,class ACTION=none,class GUARD=none>
    struct Row
    {
        typedef SOURCE  Source;
        typedef EVENT   Evt;
        typedef TARGET  Target;
        typedef ACTION  Action;
        typedef GUARD   Guard;
        // action plus guard
        typedef row_tag row_type_tag;
        template <class EVT,class FSM,class SourceState,class TargetState,class AllStates>
        static void action_call(FSM& fsm,EVT const& evt,SourceState& src,TargetState& tgt, AllStates&)
        {
            // create functor, call it
            Action()(evt,fsm,src,tgt);
        }
        template <class EVT,class FSM,class SourceState,class TargetState,class AllStates>
        static bool guard_call(FSM& fsm,EVT const& evt,SourceState& src,TargetState& tgt,AllStates&)
        {
            // create functor, call it
            return Guard()(evt,fsm,src,tgt);
        }
    };

    template<class SOURCE,class EVENT,class TARGET>
    struct Row<SOURCE,EVENT,TARGET,none,none>
    {
        typedef SOURCE  Source;
        typedef EVENT   Evt;
        typedef TARGET  Target;
        typedef none    Action;
        typedef none    Guard;
        // no action, no guard
        typedef _row_tag row_type_tag;
    };
    template<class SOURCE,class EVENT,class TARGET,class ACTION>
    struct Row<SOURCE,EVENT,TARGET,ACTION,none>
    {
        typedef SOURCE  Source;
        typedef EVENT   Evt;
        typedef TARGET  Target;
        typedef ACTION  Action;
        typedef none    Guard;
        // no guard
        typedef a_row_tag row_type_tag;
        template <class EVT,class FSM,class SourceState,class TargetState,class AllStates>
        static void action_call(FSM& fsm,EVT const& evt,SourceState& src,TargetState& tgt, AllStates&)
        {
            // create functor, call it
            Action()(evt,fsm,src,tgt);
        }
    };
    template<class SOURCE,class EVENT,class TARGET,class GUARD>
    struct Row<SOURCE,EVENT,TARGET,none,GUARD>
    {
        typedef SOURCE  Source;
        typedef EVENT   Evt;
        typedef TARGET  Target;
        typedef none    Action;
        typedef GUARD   Guard;
        // no action
        typedef g_row_tag row_type_tag;
        template <class EVT,class FSM,class SourceState,class TargetState,class AllStates>
        static bool guard_call(FSM& fsm,EVT const& evt,SourceState& src,TargetState& tgt, AllStates&)
        {
            // create functor, call it
            return Guard()(evt,fsm,src,tgt);
        }
    };
    // internal transitions
    template<class SOURCE,class EVENT,class ACTION>
    struct Row<SOURCE,EVENT,none,ACTION,none>
    {
        typedef SOURCE  Source;
        typedef EVENT   Evt;
        typedef Source  Target;
        typedef ACTION  Action;
        typedef none    Guard;
        // no guard
        typedef a_irow_tag row_type_tag;
        template <class EVT,class FSM,class SourceState,class TargetState,class AllStates>
        static void action_call(FSM& fsm,EVT const& evt,SourceState& src,TargetState& tgt, AllStates&)
        {
            // create functor, call it
            Action()(evt,fsm,src,tgt);
        }
    };
    template<class SOURCE,class EVENT,class GUARD>
    struct Row<SOURCE,EVENT,none,none,GUARD>
    {
        typedef SOURCE  Source;
        typedef EVENT   Evt;
        typedef Source  Target;
        typedef none    Action;
        typedef GUARD   Guard;
        // no action
        typedef g_irow_tag row_type_tag;
        template <class EVT,class FSM,class SourceState,class TargetState,class AllStates>
        static bool guard_call(FSM& fsm,EVT const& evt,SourceState& src,TargetState& tgt, AllStates&)
        {
            // create functor, call it
            return Guard()(evt,fsm,src,tgt);
        }
    };
    template<class SOURCE,class EVENT,class ACTION,class GUARD>
    struct Row<SOURCE,EVENT,none,ACTION,GUARD>
    {
        typedef SOURCE  Source;
        typedef EVENT   Evt;
        typedef Source  Target;
        typedef ACTION  Action;
        typedef GUARD   Guard;
        // action + guard
        typedef irow_tag row_type_tag;
        template <class EVT,class FSM,class SourceState,class TargetState,class AllStates>
        static void action_call(FSM& fsm,EVT const& evt,SourceState& src,TargetState& tgt, AllStates&)
        {
            // create functor, call it
            Action()(evt,fsm,src,tgt);
        }
        template <class EVT,class FSM,class SourceState,class TargetState,class AllStates>
        static bool guard_call(FSM& fsm,EVT const& evt,SourceState& src,TargetState& tgt, AllStates&)
        {
            // create functor, call it
            return Guard()(evt,fsm,src,tgt);
        }
    };
    template<class SOURCE,class EVENT>
    struct Row<SOURCE,EVENT,none,none,none>
    {
        typedef SOURCE  Source;
        typedef EVENT   Evt;
        typedef Source  Target;
        typedef none    Action;
        typedef none    Guard;
        // no action, no guard
        typedef _irow_tag row_type_tag;
    };
    template<class TGT>
    struct get_row_target
    {
        typedef typename TGT::Target type;
    };

    template <class EVENT,class ACTION=none,class GUARD=none>
    struct Internal
    {
        typedef EVENT   Evt;
        typedef ACTION  Action;
        typedef GUARD   Guard;
        // action plus guard
        typedef sm_i_row_tag row_type_tag;
        template <class EVT,class FSM,class SourceState,class TargetState,class AllStates>
        static void action_call(FSM& fsm,EVT const& evt,SourceState& src,TargetState& tgt, AllStates&)
        {
            // create functor, call it
            Action()(evt,fsm,src,tgt);
        }
        template <class EVT,class FSM,class SourceState,class TargetState,class AllStates>
        static bool guard_call(FSM& fsm,EVT const& evt,SourceState& src,TargetState& tgt, AllStates&)
        {
            // create functor, call it
            return Guard()(evt,fsm,src,tgt);
        }
    };

    template<class EVENT,class ACTION>
    struct Internal<EVENT,ACTION,none>
    {
        typedef EVENT   Evt;
        typedef ACTION  Action;
        typedef none    Guard;
        // no guard
        typedef sm_a_i_row_tag row_type_tag;
        template <class EVT,class FSM,class SourceState,class TargetState,class AllStates>
        static void action_call(FSM& fsm,EVT const& evt,SourceState& src,TargetState& tgt, AllStates&)
        {
            // create functor, call it
            Action()(evt,fsm,src,tgt);
        }
    };
    template<class EVENT,class GUARD>
    struct Internal<EVENT,none,GUARD>
    {
        typedef EVENT   Evt;
        typedef none    Action;
        typedef GUARD   Guard;
        // no action
        typedef sm_g_i_row_tag row_type_tag;
        template <class EVT,class FSM,class SourceState,class TargetState,class AllStates>
        static bool guard_call(FSM& fsm,EVT const& evt,SourceState& src,TargetState& tgt, AllStates&)
        {
            // create functor, call it
            return Guard()(evt,fsm,src,tgt);
        }
    };
    template<class EVENT>
    struct Internal<EVENT,none,none>
    {
        typedef EVENT   Evt;
        typedef none    Action;
        typedef none    Guard;
        // no action, no guard
        typedef sm__i_row_tag row_type_tag;
    };
    struct state_tag{};
    struct event_tag{};
    struct action_tag{};
    struct state_action_tag{};
    struct flag_tag{};
    struct config_tag{};
    struct not_euml_tag{};

    template <class Sequence>
    struct ActionSequence_
    {
        typedef Sequence sequence;
        template <class Event,class FSM,class STATE >
        struct state_action_result 
        {
            typedef void type;
        };
        template <class EVT,class FSM,class STATE>
        struct Call
        {
            Call(EVT const& evt,FSM& fsm,STATE& state):
        evt_(evt),fsm_(fsm),state_(state){}
        template <class FCT>
        void operator()(::boost::msm::wrap<FCT> const& )
        {
            FCT()(evt_,fsm_,state_);
        }
        private:
            EVT const&  evt_;
            FSM&        fsm_;
            STATE&      state_;
        };
        template <class EVT,class FSM,class SourceState,class TargetState>
        struct transition_action_result 
        {
            typedef void type;
        };
        template <class EVT,class FSM,class SourceState,class TargetState>
        struct Call2
        {
            Call2(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt):
        evt_(evt),fsm_(fsm),src_(src),tgt_(tgt){}
        template <class FCT>
        void operator()(::boost::msm::wrap<FCT> const& )
        {
            FCT()(evt_,fsm_,src_,tgt_);
        }
        private:
            EVT const & evt_;
            FSM& fsm_;
            SourceState& src_;
            TargetState& tgt_;
        };

        typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

        template <class EVT,class FSM,class STATE>
        void operator()(EVT const& evt,FSM& fsm,STATE& state)
        {
            mpl::for_each<Sequence,boost::msm::wrap< ::boost::mpl::placeholders::_1> >
                (Call<EVT,FSM,STATE>(evt,fsm,state));
        }
        template <class EVT,class FSM,class SourceState,class TargetState>
        void operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)
        {
            mpl::for_each<Sequence,boost::msm::wrap< ::boost::mpl::placeholders::_1> >
                (Call2<EVT,FSM,SourceState,TargetState>(evt,fsm,src,tgt));
        }
    };

    // functor pre-defined for basic functionality
    struct Defer 
    {
        template <class EVT,class FSM,class SourceState,class TargetState>
        void operator()(EVT const& evt,FSM& fsm,SourceState& ,TargetState& ) const
        {
            fsm.defer_event(evt);
        }
    };
}}}
#endif //BOOST_MSM_FRONT_FUNCTOR_ROW_H
