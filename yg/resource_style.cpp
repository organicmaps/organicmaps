#include "resource_style.hpp"

#include "../base/start_mem_debug.hpp"

namespace yg
{
  ResourceStyle::ResourceStyle()
  {}

  ResourceStyle::ResourceStyle(
      m2::RectU const & texRect,
      int pageID
      ) : m_cat(EGenericStyle),
      m_texRect(texRect),
      m_pageID(pageID)
  {}

  ResourceStyle::ResourceStyle(
      Category cat,
      m2::RectU const & texRect,
      int pageID)
    : m_cat(cat),
    m_texRect(texRect),
    m_pageID(pageID)
  {}

  LineStyle::LineStyle(bool isWrapped, m2::RectU const & texRect, int pageID, yg::PenInfo const & penInfo) :
    ResourceStyle(ELineStyle, texRect, pageID),
    m_isWrapped(isWrapped),
    m_isSolid(penInfo.m_isSolid),
    m_penInfo(penInfo)
  {
    if (m_isSolid)
    {
      m_borderColorPixel = m_centerColorPixel = m2::PointU(texRect.minX() + 1, texRect.minY() + 1);
    }
    else
    {
      double firstDashOffset = penInfo.firstDashOffset();
      m_centerColorPixel = m2::PointU(static_cast<uint32_t>(firstDashOffset + texRect.minX() + 3),
                                      static_cast<uint32_t>(texRect.minY() + texRect.SizeY() / 2.0));
      m_borderColorPixel = m2::PointU(static_cast<uint32_t>(firstDashOffset + texRect.minX() + 3),
                                      static_cast<uint32_t>(texRect.minY() + 1));
    }
  }

  double LineStyle::geometryTileLen() const
  {
    return m_texRect.SizeX() - 2;
  }

  double LineStyle::geometryTileWidth() const
  {
    return m_texRect.SizeY() - 2;
  }

  double LineStyle::rawTileLen() const
  {
    return m_texRect.SizeX() - 4;
  }

  double LineStyle::rawTileWidth() const
  {
    return m_texRect.SizeY() - 4;
  }

  CharStyle::CharStyle(m2::RectU const & texRect, int pageID, int8_t xOffset, int8_t yOffset, int8_t xAdvance)
    : ResourceStyle(ECharStyle, texRect, pageID), m_xOffset(xOffset), m_yOffset(yOffset), m_xAdvance(xAdvance)
  {}

  PointStyle::PointStyle(m2::RectU const & texRect, int pageID, string const & styleName)
    : ResourceStyle(EPointStyle, texRect, pageID), m_styleName(styleName)
  {}

  GenericStyle::GenericStyle(m2::RectU const & texRect, int pageID)
    : ResourceStyle(EGenericStyle, texRect, pageID)
  {}
}
