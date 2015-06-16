// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_BASE_DYNAMIC_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_BASE_DYNAMIC_HPP

#include <string>


#include <boost/geometry/extensions/gis/projections/projection.hpp>

namespace boost { namespace geometry { namespace projections
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

// Base-virtual-forward
template <typename C, typename LL, typename XY, typename P>
class base_v_f : public projection<LL, XY>
{
protected:

    typedef typename projection<LL, XY>::LL_T LL_T;
    typedef typename projection<LL, XY>::XY_T XY_T;

public:

    base_v_f(P const& params) : m_proj(params) {}

    virtual P const& params() const { return m_proj.params(); }

    virtual P& mutable_params() { return m_proj.mutable_params(); }

    virtual bool forward(LL const& ll, XY& xy) const
    {
        return m_proj.forward(ll, xy);
    }

    virtual void fwd(LL_T& lp_lon, LL_T& lp_lat, XY_T& xy_x, XY_T& xy_y) const
    {
        m_proj.fwd(lp_lon, lp_lat, xy_x, xy_y);
    }

    virtual bool inverse(XY const& , LL& ) const
    {
        // exception?
        return false;
    }
    virtual void inv(XY_T& , XY_T& , LL_T& , LL_T& ) const
    {
        // exception?
    }

    virtual std::string name() const
    {
        return m_proj.name();
    }

protected:

    C m_proj;
};

// Base-virtual-forward/inverse
template <typename C, typename LL, typename XY, typename P>
class base_v_fi : public base_v_f<C, LL, XY, P>
{
private:

    typedef typename base_v_f<C, LL, XY, P>::LL_T LL_T;
    typedef typename base_v_f<C, LL, XY, P>::XY_T XY_T;

public :

    base_v_fi(P const& params) : base_v_f<C, LL, XY, P>(params) {}

    virtual bool inverse(XY const& xy, LL& ll) const
    {
        return this->m_proj.inverse(xy, ll);
    }

    void inv(XY_T& xy_x, XY_T& xy_y, LL_T& lp_lon, LL_T& lp_lat) const
    {
        this->m_proj.inv(xy_x, xy_y, lp_lon, lp_lat);
    }
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_BASE_DYNAMIC_HPP
