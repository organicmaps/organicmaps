/*
 [auto_generated]
 boost/numeric/odeint/integrate/detail/integrate_times.hpp

 [begin_description]
 Default integrate times implementation.
 [end_description]

 Copyright 2011-2012 Mario Mulansky
 Copyright 2012 Karsten Ahnert
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_INTEGRATE_DETAIL_INTEGRATE_TIMES_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_INTEGRATE_DETAIL_INTEGRATE_TIMES_HPP_INCLUDED

#include <stdexcept>

#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/numeric/odeint/util/unwrap_reference.hpp>
#include <boost/numeric/odeint/stepper/controlled_step_result.hpp>
#include <boost/numeric/odeint/util/detail/less_with_sign.hpp>


namespace boost {
namespace numeric {
namespace odeint {
namespace detail {



/*
 * integrate_times for simple stepper
 */
template< class Stepper , class System , class State , class TimeIterator , class Time , class Observer >
size_t integrate_times(
        Stepper stepper , System system , State &start_state ,
        TimeIterator start_time , TimeIterator end_time , Time dt ,
        Observer observer , stepper_tag
)
{
    typename odeint::unwrap_reference< Observer >::type &obs = observer;
    typename odeint::unwrap_reference< Stepper >::type &st = stepper;
    typedef typename unit_value_type<Time>::type time_type;

    size_t steps = 0;
    Time current_dt = dt;
    while( true )
    {
        Time current_time = *start_time++;
        obs( start_state , current_time );
        if( start_time == end_time )
            break;
        while( less_with_sign( current_time , static_cast<time_type>(*start_time) , current_dt ) )
        {
            current_dt = min_abs( dt , *start_time - current_time );
            st.do_step( system , start_state , current_time , current_dt );
            current_time += current_dt;
            steps++;
        }
    }
    return steps;
}

/*
 * integrate_times for controlled stepper
 */
template< class Stepper , class System , class State , class TimeIterator , class Time , class Observer >
size_t integrate_times(
        Stepper stepper , System system , State &start_state ,
        TimeIterator start_time , TimeIterator end_time , Time dt ,
        Observer observer , controlled_stepper_tag
)
{
    typename odeint::unwrap_reference< Observer >::type &obs = observer;
    typename odeint::unwrap_reference< Stepper >::type &st = stepper;
    typedef typename unit_value_type<Time>::type time_type;

    const size_t max_attempts = 1000;
    const char *error_string = "Integrate adaptive : Maximal number of iterations reached. A step size could not be found.";
    size_t steps = 0;
    while( true )
    {
        size_t fail_steps = 0;
        Time current_time = *start_time++;
        obs( start_state , current_time );
        if( start_time == end_time )
            break;
        while( less_with_sign( current_time , static_cast<time_type>(*start_time) , dt ) )
        {
            // adjust stepsize to end up exactly at the observation point
            Time current_dt = min_abs( dt , *start_time - current_time );
            if( st.try_step( system , start_state , current_time , current_dt ) == success )
            {
                ++steps;
                // continue with the original step size if dt was reduced due to observation
                dt = max_abs( dt , current_dt );
            }
            else
            {
                ++fail_steps;
                dt = current_dt;
            }
            if( fail_steps == max_attempts ) BOOST_THROW_EXCEPTION( std::overflow_error( error_string ));
        }
    }
    return steps;
}

/*
 * integrate_times for dense output stepper
 */
template< class Stepper , class System , class State , class TimeIterator , class Time , class Observer >
size_t integrate_times(
        Stepper stepper , System system , State &start_state ,
        TimeIterator start_time , TimeIterator end_time , Time dt ,
        Observer observer , dense_output_stepper_tag
)
{
    typename odeint::unwrap_reference< Observer >::type &obs = observer;
    typename odeint::unwrap_reference< Stepper >::type &st = stepper;
    typedef typename unit_value_type<Time>::type time_type;

    if( start_time == end_time )
        return 0;

    Time last_time_point = static_cast<time_type>(*(end_time-1));

    st.initialize( start_state , *start_time , dt );
    obs( start_state , *start_time++ );

    size_t count = 0;
    while( start_time != end_time )
    {
        while( ( start_time != end_time ) && less_eq_with_sign( static_cast<time_type>(*start_time) , st.current_time() , st.current_time_step() ) )
        {
            st.calc_state( *start_time , start_state );
            obs( start_state , *start_time );
            start_time++;
        }

        // we have not reached the end, do another real step
        if( less_eq_with_sign( st.current_time() + st.current_time_step() ,
                               last_time_point ,
                               st.current_time_step() ) )
        {
            st.do_step( system );
            ++count;
        }
        else if( start_time != end_time )
        { // do the last step ending exactly on the end point
            st.initialize( st.current_state() , st.current_time() , last_time_point - st.current_time() );
            st.do_step( system );
            ++count;
        }
    }
    return count;
}


} // namespace detail
} // namespace odeint
} // namespace numeric
} // namespace boost


#endif // BOOST_NUMERIC_ODEINT_INTEGRATE_DETAIL_INTEGRATE_ADAPTIVE_HPP_INCLUDED
