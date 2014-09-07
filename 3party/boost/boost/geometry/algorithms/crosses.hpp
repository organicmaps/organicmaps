// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2014 Samuel Debionne, Grenoble, France.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014 Oracle and/or its affiliates.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

#ifndef BOOST_GEOMETRY_ALGORITHMS_CROSSES_HPP
#define BOOST_GEOMETRY_ALGORITHMS_CROSSES_HPP

#include <cstddef>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/core/access.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/algorithms/detail/relate/relate.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Geometry1,
    typename Geometry2,
    typename Tag1 = typename tag<Geometry1>::type,
    typename Tag2 = typename tag<Geometry2>::type
>
struct crosses
    : detail::relate::relate_base
        <
            detail::relate::static_mask_crosses_type,
            Geometry1,
            Geometry2
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


namespace resolve_variant
{
    template <typename Geometry1, typename Geometry2>
    struct crosses
    {
        static inline bool
        apply(
              const Geometry1& geometry1,
              const Geometry2& geometry2)
        {
            concept::check<Geometry1 const>();
            concept::check<Geometry2 const>();
            
            return dispatch::crosses<Geometry1, Geometry2>::apply(geometry1, geometry2);
        }
    };
    
    
    template <BOOST_VARIANT_ENUM_PARAMS(typename T), typename Geometry2>
    struct crosses<variant<BOOST_VARIANT_ENUM_PARAMS(T)>, Geometry2>
    {
        struct visitor: static_visitor<bool>
        {
            Geometry2 const& m_geometry2;
            
            visitor(Geometry2 const& geometry2)
            : m_geometry2(geometry2)
            {}
            
            template <typename Geometry1>
            result_type operator()(Geometry1 const& geometry1) const
            {
                return crosses
                <
                    Geometry1,
                    Geometry2
                >::apply
                (geometry1, m_geometry2);
            }
        };
        
        static inline bool
        apply(variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry1,
              Geometry2 const& geometry2)
        {
            return apply_visitor(visitor(geometry2), geometry1);
        }
    };
    
    
    template <typename Geometry1, BOOST_VARIANT_ENUM_PARAMS(typename T)>
    struct crosses<Geometry1, variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
    {
        struct visitor: static_visitor<bool>
        {
            Geometry1 const& m_geometry1;
            
            visitor(Geometry1 const& geometry1)
            : m_geometry1(geometry1)
            {}
            
            template <typename Geometry2>
            result_type operator()(Geometry2 const& geometry2) const
            {
                return crosses
                <
                    Geometry1,
                    Geometry2
                >::apply
                (m_geometry1, geometry2);
            }
        };
        
        static inline bool
        apply(
              Geometry1 const& geometry1,
              const variant<BOOST_VARIANT_ENUM_PARAMS(T)>& geometry2)
        {
            return apply_visitor(visitor(geometry1), geometry2);
        }
    };
    
    
    template <BOOST_VARIANT_ENUM_PARAMS(typename A), BOOST_VARIANT_ENUM_PARAMS(typename B)>
    struct crosses<variant<BOOST_VARIANT_ENUM_PARAMS(A)>, variant<BOOST_VARIANT_ENUM_PARAMS(B)> >
    {
        struct visitor: static_visitor<bool>
        {
            template <typename Geometry1, typename Geometry2>
            result_type operator()(
                                   Geometry1 const& geometry1,
                                   Geometry2 const& geometry2) const
            {
                return crosses
                <
                Geometry1,
                Geometry2
                >::apply
                (geometry1, geometry2);
            }
        };
        
        static inline bool
        apply(
              const variant<BOOST_VARIANT_ENUM_PARAMS(A)>& geometry1,
              const variant<BOOST_VARIANT_ENUM_PARAMS(B)>& geometry2)
        {
            return apply_visitor(visitor(), geometry1, geometry2);
        }
    };
    
} // namespace resolve_variant
    
    
/*!
\brief \brief_check2{crosses}
\ingroup crosses
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\return \return_check2{crosses}

\qbk{[include reference/algorithms/crosses.qbk]}
*/
template <typename Geometry1, typename Geometry2>
inline bool crosses(Geometry1 const& geometry1, Geometry2 const& geometry2)
{
    return resolve_variant::crosses<Geometry1, Geometry2>::apply(geometry1, geometry2);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_CROSSES_HPP
