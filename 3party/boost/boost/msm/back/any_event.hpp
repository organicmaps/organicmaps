// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_BACK_ANY_EVENT_H
#define BOOST_MSM_BACK_ANY_EVENT_H

#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/msm/back/common_types.hpp>

namespace boost { namespace msm { namespace back
{
class placeholder
{
public:
    virtual ::boost::msm::back::HandledEnum process_event() const =0;
};
template<class EventType,class FsmType>
class holder : public placeholder
{
public:
    holder(EventType const& evt, FsmType& fsm){}
    virtual ::boost::msm::back::HandledEnum process_event() const
    {
        //default. Will not be called
        return HANDLED_TRUE;
    }
private:
};

class any_event
{
public:
    template <class EventType,class FsmType>
    any_event(EventType const& evt,FsmType& fsm):content_(new holder<EventType,FsmType>(evt,fsm)){}
    ::boost::msm::back::HandledEnum process_event() const
    {
        return content_->process_event();
    }
private:

    ::boost::scoped_ptr<placeholder> content_;
};

}}}

#endif //BOOST_MSM_BACK_ANY_EVENT_H

