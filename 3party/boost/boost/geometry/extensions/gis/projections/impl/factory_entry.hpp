// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_FACTORY_ENTRY_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_FACTORY_ENTRY_HPP

#include <string>

#include <boost/geometry/extensions/gis/projections/projection.hpp>

namespace boost { namespace geometry { namespace projections
{

namespace detail
{

template <typename LL, typename XY, typename P>
class factory_entry
{
public:

    virtual ~factory_entry() {}
    virtual projection<LL, XY>* create_new(P const& par) const = 0;
};

template <typename LL, typename XY, typename P>
class base_factory
{
public:

    virtual ~base_factory() {}
    virtual void add_to_factory(std::string const& name, factory_entry<LL, XY, P>* sub) = 0;
};

} // namespace detail
}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_FACTORY_ENTRY_HPP
