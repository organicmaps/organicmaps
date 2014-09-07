/*
 [auto_generated]
 boost/numeric/odeint/integrate/detail/integrate_adaptive.hpp

 [begin_description]
 Default Integrate adaptive implementation.
 [end_description]

 Copyright 2011-2013 Karsten Ahnert
 Copyright 2011-2012 Mario Mulansky
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_INTEGRATE_DETAIL_INTEGRATE_ADAPTIVE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_INTEGRATE_DETAIL_INTEGRATE_ADAPTIVE_HPP_INCLUDED

#include <stdexcept>

#include <boost/throw_exception.hpp>

#include <boost/numeric/odeint/stepper/stepper_categories.hpp>
#include <boost/numeric/odeint/stepper/controlled_step_result.hpp>
#include <boost/numeric/odeint/integrate/detail/integrate_const.hpp>
#include <boost/numeric/odeint/util/bind.hpp>
#include <boost/numeric/odeint/util/unwrap_reference.hpp>
#include <boost/numeric/odeint/util/copy.hpp>

#include <boost/numeric/odeint/util/detail/less_with_sign.hpp>


#include <iostream>

namespace boost {
namespace numeric {
namespace odeint {
namespace detail {

// forward declaration
template< class Stepper , class System , class State , class Time , class Observer>
size_t integrate_const(
        Stepper stepper , System system , State &start_state ,
        Time start_time , Time end_time , Time dt ,
        Observer observer , stepper_tag );

/*
 * integrate_adaptive for simple stepper is basically an integrate_const + some last step
 */
template< class Stepper , class System , class State , class Time , class Observer >
size_t integrate_adaptive(
        Stepper stepper , System system , State &start_state ,
        Time start_time , Time end_time , Time dt ,
        Observer observer , stepper_tag
)
{
    size_t steps = detail::integrate_const( stepper , system , start_state , start_time ,
                                            end_time , dt , observer , stepper_tag() );
    typename odeint::unwrap_reference< Observer >::type &obs = observer;
    typename odeint::unwrap_reference< Stepper >::type &st = stepper;

    Time end = start_time + dt*steps;
    if( less_with_sign( end , end_time , dt ) )
    {   //make a last step to end exactly at end_time
        st.do_step( system , start_state , end , end_time - end );
        steps++;
        obs( start_state , end_time );
    }
    return steps;
}


/*
 * classical integrate adaptive
 */
template< class Stepper , class System , class State , class Time , class Observer >
size_t integrate_adaptive(
        Stepper stepper , System system , State &start_state ,
        Time &start_time , Time end_time , Time &dt ,
        Observer observer , controlled_stepper_tag
)
{
    typename odeint::unwrap_reference< Observer >::type &obs = observer;
    typename odeint::unwrap_reference< Stepper >::type &st = stepper;

    const size_t max_attempts = 1000;
    const char *error_string = "Integrate adaptive : Maximal number of iterations reached. A step size could not be found.";
    size_t count = 0;
    while( less_with_sign( start_time , end_time , dt ) )
    {
        obs( start_state , start_time );
        if( less_with_sign( end_time , static_cast<Time>(start_time + dt) , dt ) )
        {
            dt = end_time - start_time;
        }

        size_t trials = 0;
        controlled_step_result res = success;
        do
        {
            res = st.try_step( system , start_state , start_time , dt );
            ++trials;
        }
        while( ( res == fail ) && ( trials < max_attempts ) );
        if( trials == max_attempts ) BOOST_THROW_EXCEPTION( std::overflow_error( error_string ) );

        ++count;
    }
    obs( start_state , start_time );
    return count;
}


/*
 * integrate adaptive for dense output steppers
 *
 * step size control is used if the stepper supports it
 */
template< class Stepper , class System , class State , class Time , class Observer >
size_t integrate_adaptive(
        Stepper stepper , System system , State &start_state ,
        Time start_time , Time end_time , Time dt ,
        Observer observer , dense_output_stepper_tag )
{
    typename odeint::unwrap_reference< Observer >::type &obs = observer;
    typename odeint::unwrap_reference< Stepper >::type &st = stepper;

    size_t count = 0;
    st.initialize( start_state , start_time , dt );

    while( less_with_sign( st.current_time() , end_time , st.current_time_step() ) )
    {
        while( less_eq_with_sign( static_cast<Time>(st.current_time() + st.current_time_step()) ,
               end_time ,
               st.current_time_step() ) )
        {   //make sure we don't go beyond the end_time
            obs( st.current_state() , st.current_time() );
            st.do_step( system );
            ++count;
        }
        // calculate time step to arrive exactly at end time
        st.initialize( st.current_state() , st.current_time() , static_cast<Time>(end_time - st.current_time()) );
    }
    obs( st.current_state() , st.current_time() );
    // overwrite start_state with the final point
    boost::numeric::odeint::copy( st.current_state() , start_state );
    return count;
}




} // namespace detail
} // namespace odeint
} // namespace numeric
} // namespace boost


#endif // BOOST_NUMERIC_ODEINT_INTEGRATE_DETAIL_INTEGRATE_ADAPTIVE_HPP_INCLUDED
