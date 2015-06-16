// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHAPE_READER_HPP
#define BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHAPE_READER_HPP

#include <fstream>
#include "shapefil.h"


#include <boost/noncopyable.hpp>
#include <boost/type_traits/promote.hpp>

#include <boost/geometry/extensions/gis/io/shapelib/shp_read_object.hpp>


namespace boost { namespace geometry
{



namespace detail
{


template<typename Geometry>
class shape_reader : public boost::noncopyable
{
public :
    shape_reader(std::string const& name)
    {
        m_shp = ::SHPOpen((name + ".shp").c_str(), "rb");
        m_dbf = ::DBFOpen((name + ".dbf").c_str(), "rb");

        if (m_shp == NULL || m_dbf == NULL)
        {
            throw shapelib_file_open_exception(name);
        }

        double adfMinBound[4], adfMaxBound[4];
        SHPGetInfo(m_shp, &m_count, &m_shape_type, adfMinBound, adfMaxBound );

    }
    virtual ~shape_reader()
    {
        if (m_shp) ::SHPClose(m_shp);
        if (m_dbf) ::DBFClose(m_dbf);
    }

    inline int count() const { return m_count; }




private :
    ::SHPHandle m_shp;
    ::DBFHandle m_dbf;
    int m_count;
    int m_shape_type;

};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHAPE_READER_HPP
