#ifndef RESOURCE_STYLE_HPP
#define RESOURCE_STYLE_HPP

#include "../geometry/rect2d.hpp"
#include "pen_info.hpp"

#include "../base/start_mem_debug.hpp"

namespace yg
{
  struct ResourceStyle
  {
    enum Category
    {
        ELineStyle = 1,
        ECharStyle,
        EPointStyle,
        EGenericStyle
    };

    Category m_cat;
    m2::RectU m_texRect;
    int m_pageID;

    ResourceStyle();
    ResourceStyle(m2::RectU const & texRect, int pageID);

  protected:
    ResourceStyle(Category cat, m2::RectU const & texRect, int pageID);
  };

  struct LineStyle : public ResourceStyle
  {
    bool m_isWrapped;
    bool m_isSolid;
    yg::PenInfo m_penInfo;
    m2::PointU m_centerColorPixel;
    m2::PointU m_borderColorPixel;
    LineStyle(bool isWrapped, m2::RectU const & texRect, int pageID, yg::PenInfo const & penInfo);

    /// with antialiasing zones
    double geometryTileLen() const;
    double geometryTileWidth() const;

    /// without antialiasing zones
    double rawTileLen() const;
    double rawTileWidth() const;

  };

  struct CharStyle : public ResourceStyle
  {
    int8_t m_xOffset;
    int8_t m_yOffset;
    int8_t m_xAdvance;

    CharStyle(m2::RectU const & texRect, int pageID, int8_t xOffset, int8_t yOffset, int8_t xAdvance);
  };

  struct PointStyle : public ResourceStyle
  {
    string m_styleName;
    PointStyle(m2::RectU const & texRect, int pageID, string const & styleName);
  };

  struct GenericStyle : public ResourceStyle
  {
    GenericStyle(m2::RectU const & texRect, int pageID);
  };
}

#include "../base/start_mem_debug.hpp"

#endif // RESOURCE_STYLE_HPP
