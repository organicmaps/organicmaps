/// @author Alex Zolotarev
#pragma once

#include "graphics/resource_style.hpp"
#include "geometry/rect2d.hpp"
#include "base/base.hpp"
#include "std/string.hpp"
#include "coding/strutil.hpp"

#include "base/start_mem_debug.hpp"

/// @example
/// <font>
///   <chars count="256">
///     <char id="1024" x="341" y="0" width="7" height="13" xoffset="1" yoffset="0" xadvance="9" page="0" chnl="15" />
///     ...
///   </chars>
/// </font>
/// <skin>
///   <chars count="55">
///     <char id="666" x="341" y="0" width="7" height="13" />
///     ...
///   </chars>
/// </skin>

namespace yg
{
  /// used to parse xml skin description file in BMFont app out format
  template<class TSkinInserter, class TFontInserter>
  class SkinXmlParser
  {
  private:
    enum TMode
    {
      EModeUnknown,
      EModeSkin,
      EModeFont
    };

    TMode m_mode;
    bool m_ready;

    uint16_t m_x;
    uint16_t m_y;
    uint16_t m_width;
    uint16_t m_height;
    int8_t m_xOffset;
    int8_t m_yOffset;
    int8_t m_xAdvance;
    int m_id;
    /// How many pixels from top left to baseline (down)
    int m_baseline;
    /// width of the font texture
    int m_scaleW;
    /// height of the font texture
    int m_scaleH;
    TSkinInserter m_skinInserter;
    TFontInserter m_fontInserter;

  public:

    SkinXmlParser(TSkinInserter skinInserter, TFontInserter fontInserter)
      : m_mode(EModeUnknown), m_ready(false), m_baseline(0), m_scaleW(0), m_scaleH(0),
        m_skinInserter(skinInserter), m_fontInserter(fontInserter)
    {}

    int FontTextureWidth() const { return m_scaleW; }
    int FontTextureHeight() const { return m_scaleH; }
    int BaseLineOffset() const { return m_baseline; }

    bool Push(string const & element)
    {
      if (m_mode == EModeUnknown)
      {
        if (element == "skin")
          m_mode = EModeSkin;
        else if (element == "font")
          m_mode = EModeFont;
      }
      else if (element == "char")
        m_ready = true;
      return true;
    }

    void AddAttr(string const & attribute, string const & value)
    {
      if (m_ready)
      {
        if (attribute == "id")
          m_id = StrToInt(value);
        else if (attribute == "x")
          m_x = static_cast<uint16_t>(StrToInt(value));
        else if (attribute == "y")
          m_y = static_cast<uint16_t>(StrToInt(value));
        else if (attribute == "width")
          m_width = static_cast<uint16_t>(StrToInt(value));
        else if (attribute == "height")
          m_height = static_cast<uint16_t>(StrToInt(value));
        else if (attribute == "xoffset")
          m_xOffset = static_cast<int8_t>(StrToInt(value));
        else if (attribute == "yoffset")
          m_yOffset = static_cast<int8_t>(StrToInt(value));
        else if (attribute == "xadvance")
          m_xAdvance = static_cast<int8_t>(StrToInt(value));
      }
      else
      {
        if (attribute == "base")
          m_baseline = StrToInt(value);
        else if (attribute == "scaleW")
          m_scaleW = StrToInt(value);
        else if (attribute == "scaleH")
          m_scaleH = StrToInt(value);
      }
    }

    void Pop(string const & element)
    {
      if (m_ready)
      {
        switch (m_mode)
        {
        case EModeSkin:
          m_skinInserter =
              make_pair(m_id, shared_ptr<ResourceStyle>(new ResourceStyle(m2::RectU(m_x, m_y, m_x + m_width, m_y + m_height))));
          break;
        case EModeFont:
          m_fontInserter =
              make_pair(m_id, shared_ptr<FontStyle>(new FontStyle(m2::RectU(m_x, m_y, m_x + m_width, m_y + m_height),
                                                m_xOffset, m_baseline - m_yOffset - m_height, m_xAdvance)));
          break;
        default:
          break;
        }
        m_ready = false;
      }
      else  if (element == "skin" || element == "font")
        m_mode = EModeUnknown;
    }
  };
}
