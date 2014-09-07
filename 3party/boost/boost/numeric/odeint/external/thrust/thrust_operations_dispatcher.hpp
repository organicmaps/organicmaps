/*
  [auto_generated]
  boost/numeric/odeint/external/thrust/thrust_operations_dispatcher.hpp

  [begin_description]
  operations_dispatcher specialization for thrust
  [end_description]

  Copyright 2013 Karsten Ahnert
  Copyright 2013 Mario Mulansky

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_OPERATIONS_DISPATCHER_HPP_DEFINED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_OPERATIONS_DISPATCHER_HPP_DEFINED

#include <thrust/host_vector.h>
#include <thrust/device_vector.h>

#include <boost/numeric/odeint/external/thrust/thrust_operations.hpp>
#include <boost/numeric/odeint/algebra/operations_dispatcher.hpp>


namespace boost {
namespace numeric {
namespace odeint {

// specialization for thrust host_vector
template< class T , class A >
struct operations_dispatcher< thrust::host_vector< T , A > >
{
    typedef thrust_operations operations_type;
};

// specialization for thrust device_vector
template< class T , class A >
struct operations_dispatcher< thrust::device_vector< T , A > >
{
    typedef thrust_operations operations_type;
};

} // namespace odeint
} // namespace numeric
} // namespace boost


#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_ALGEBRA_DISPATCHER_HPP_DEFINED

