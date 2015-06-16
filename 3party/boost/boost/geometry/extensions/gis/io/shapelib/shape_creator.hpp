// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHAPE_CREATOR_HPP
#define BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHAPE_CREATOR_HPP

#include <fstream>
#include "shapefil.h"

#include <boost/noncopyable.hpp>
#include <boost/type_traits/promote.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>

#include <boost/geometry/extensions/gis/io/shapelib/shp_create_object.hpp>
#include <boost/geometry/extensions/gis/io/shapelib/shp_create_object_multi.hpp>
#include <boost/geometry/extensions/gis/io/shapelib/dbf_write_attribute.hpp>

namespace boost { namespace geometry
{

class shapelib_file_create_exception : public geometry::exception
{
public:

    inline shapelib_file_create_exception(std::string const& filename)
        : m_filename(filename)
    {}

    virtual char const* what() const throw()
    {
        return m_filename.c_str();
    }
private :
    std::string m_filename;
};

namespace detail
{

template <typename Tag>
struct SHPType
{
};

template <> struct SHPType<point_tag> { static int const value = SHPT_POINT; };
template <> struct SHPType<segment_tag> { static int const value = SHPT_ARC; };
template <> struct SHPType<linestring_tag> { static int const value = SHPT_ARC; };
template <> struct SHPType<polygon_tag> { static int const value = SHPT_POLYGON; };
template <> struct SHPType<ring_tag> { static int const value = SHPT_POLYGON; };
template <> struct SHPType<box_tag> { static int const value = SHPT_POLYGON; };

template <> struct SHPType<multi_point_tag> { static int const value = SHPT_MULTIPOINT; };
template <> struct SHPType<multi_linestring_tag> { static int const value = SHPT_ARC; };
template <> struct SHPType<multi_polygon_tag> { static int const value = SHPT_POLYGON; };

} // namespace detail

template
<
    typename Geometry,
    int ShapeType = detail::SHPType
        <
            typename geometry::tag<Geometry>::type
        >::value
>
class shape_creator : public boost::noncopyable
{
public :
    shape_creator(std::string const& name)
    {
        m_shp = ::SHPCreate((name + ".shp").c_str(), ShapeType);
        m_dbf = ::DBFCreate((name + ".dbf").c_str());
        m_prj_name = name + ".prj";

        if (m_shp == NULL || m_dbf == NULL)
        {
            throw shapelib_file_create_exception(name);
        }
    }

    virtual ~shape_creator()
    {
        if (m_shp) ::SHPClose(m_shp);
        if (m_dbf) ::DBFClose(m_dbf);
    }

    // Returns: index in shapefile
    inline int AddShape(Geometry const& geometry)
    {
        // Note: we MIGHT design a small wrapper class which destroys in destructor
        ::SHPObject* obj = SHPCreateObject(geometry);
        int result = SHPWriteObject(m_shp, -1, obj );
        ::SHPDestroyObject( obj );
        return result;
    }

    template <typename T>
    inline void AddField(std::string const& name, int width = 16, int decimals = 0)
    {
        ::DBFAddField(m_dbf, name.c_str(),
            detail::DBFFieldType
                <
                    typename boost::promote<T>::type
                >::value,
            width, decimals);
    }

    template <typename T>
    inline void WriteField(int row_index, int field_index, T const& value)
    {
        detail::DBFWriteAttribute
            <
                typename boost::promote<T>::type
            >::apply(m_dbf, row_index, field_index, value);
    }

    inline void SetSrid(int srid)
    {
        if (srid == 28992)
        {
            std::ofstream out(m_prj_name.c_str());
            out << "PROJCS[\"RD_New\""
                << ",GEOGCS[\"GCS_Amersfoort\""
                << ",DATUM[\"D_Amersfoort\""
                << ",SPHEROID[\"Bessel_1841\",6377397.155,299.1528128]]"
                << ",PRIMEM[\"Greenwich\",0]"
                << ",UNIT[\"Degree\",0.0174532925199432955]]"
                << ",PROJECTION[\"Double_Stereographic\"]"
                << ",PARAMETER[\"False_Easting\",155000]"
                << ",PARAMETER[\"False_Northing\",463000]"
                << ",PARAMETER[\"Central_Meridian\",5.38763888888889]"
                << ",PARAMETER[\"Scale_Factor\",0.9999079]"
                << ",PARAMETER[\"Latitude_Of_Origin\",52.15616055555555]"
                << ",UNIT[\"Meter\",1]]"
                << std::endl;
        }
    }

private :
    ::SHPHandle m_shp;
    ::DBFHandle m_dbf;
    std::string m_prj_name;

};

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHAPE_CREATOR_HPP
