#include "proto_to_yg_styles.hpp"

#ifdef OMIM_PRODUCTION
  #include "../indexer/drules_struct_lite.pb.h"
#else
  #include "../indexer/drules_struct.pb.h"
#endif


#include "../std/algorithm.hpp"


namespace
{
  yg::Color ConvertColor(int c)
  {
    return yg::Color::fromXRGB(c, 255 - (c >> 24));
  }

  double ConvertWidth(double w, double scale)
  {
    return max(w, 1.0) * scale;
  }
}


void ConvertStyle(LineDefProto const * pSrc, double scale, yg::PenInfo & dest)
{
  double offset = 0.0;
  vector<double> v;

  if (pSrc->has_dashdot())
  {
    DashDotProto const & dd = pSrc->dashdot();

    int const count = dd.dd_size();
    v.reserve(count);
    for (int i = 0; i < count; ++i)
      v.push_back(dd.dd(i) * scale);

    if (dd.has_offset())
      offset = dd.offset() * scale;
  }

  dest = yg::PenInfo(
        ConvertColor(pSrc->color()),
        ConvertWidth(pSrc->width(), scale),
        v.empty() ? 0 : &v[0], v.size(), offset);
}

void ConvertStyle(AreaRuleProto const * pSrc, yg::Color & dest)
{
  dest = ConvertColor(pSrc->color());
}

void ConvertStyle(SymbolRuleProto const * pSrc, string & dest)
{
  dest = pSrc->name();
}

void ConvertStyle(CircleRuleProto const * pSrc, double scale, yg::CircleInfo & dest)
{
  dest = yg::CircleInfo(min(max(pSrc->radius(), 3.0), 6.0) * scale,
                        ConvertColor(pSrc->color()));

  if (pSrc->has_border())
  {
    yg::PenInfo pen;
    ConvertStyle(&(pSrc->border()), scale, pen);

    dest.m_isOutlined = true;
    dest.m_outlineColor = pen.m_color;
    dest.m_outlineWidth = pen.m_w;
  }
}

void ConvertStyle(CaptionDefProto const * pSrc, double scale, yg::FontDesc & dest)
{
  uint8_t const h = max(static_cast<int>(pSrc->height() * scale),
                        static_cast<int>(8 * scale));    // replace 12 to 8 as it defined in drawing rules

  dest = yg::FontDesc(h, ConvertColor(pSrc->color()));

  if (pSrc->has_stroke_color())
  {
    dest.m_isMasked = true;
    dest.m_maskColor = ConvertColor(pSrc->stroke_color());
  }
}

uint8_t GetFontSize(CaptionDefProto const * pSrc)
{
  return pSrc->height();
}
