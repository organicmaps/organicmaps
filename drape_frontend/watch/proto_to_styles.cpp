#include "proto_to_styles.hpp"

#include "indexer/drules_include.hpp"

#include "std/algorithm.hpp"

namespace
{

double ConvertWidth(double w, double scale)
{
  return max(w * scale, 1.0);
}

}

namespace df
{
namespace watch
{

dp::Color ConvertColor(uint32_t c)
{
  return dp::Extract(c, 255 - (c >> 24));
}

void ConvertStyle(LineDefProto const * pSrc, double scale, PenInfo & dest)
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

  dest = PenInfo(ConvertColor(pSrc->color()),
                 ConvertWidth(pSrc->width(), scale),
                 v.empty() ? 0 : &v[0], v.size(), offset);

  if (pSrc->has_pathsym())
  {
    PathSymProto const & ps = pSrc->pathsym();

    dest.m_step = ps.step() * scale;
    dest.m_icon.m_name = ps.name();

    if (ps.has_offset())
      dest.m_offset = ps.offset() * scale;
  }

  if (pSrc->has_join())
  {
    switch (pSrc->join())
    {
    case ROUNDJOIN:
      dest.m_join = dp::RoundJoin;
      break;
    case BEVELJOIN:
      dest.m_join = dp::BevelJoin;
      break;
    case NOJOIN:
      dest.m_join = dp::MiterJoin;
      break;
    default:
      break;
    }
  }

  if (pSrc->has_cap())
  {
    switch (pSrc->cap())
    {
    case ROUNDCAP:
      dest.m_cap = dp::RoundCap;
      break;
    case BUTTCAP:
      dest.m_cap = dp::ButtCap;
      break;
    case SQUARECAP:
      dest.m_cap = dp::SquareCap;
      break;
    default:
      break;
    }
  }
}

void ConvertStyle(AreaRuleProto const * pSrc, BrushInfo & dest)
{
  dest.m_color = ConvertColor(pSrc->color());
}

void ConvertStyle(SymbolRuleProto const * pSrc, IconInfo & dest)
{
  dest.m_name = pSrc->name();
}

void ConvertStyle(CircleRuleProto const * pSrc, double scale, CircleInfo & dest)
{
  dest = CircleInfo(pSrc->radius() * scale, ConvertColor(pSrc->color()));

  if (pSrc->has_border())
  {
    PenInfo pen;
    ConvertStyle(&(pSrc->border()), scale, pen);

    dest.m_isOutlined = true;
    dest.m_outlineColor = pen.m_color;
    dest.m_outlineWidth = pen.m_w;
  }
}

void ConvertStyle(CaptionDefProto const * pSrc, double scale, dp::FontDecl & dest, m2::PointD & offset)
{
  // fonts smaller than 8px look "jumpy" on LDPI devices
  uint8_t const h = max(8, static_cast<int>(pSrc->height() * scale));

  offset = m2::PointD(0, 0);
  if (pSrc->has_offset_x())
    offset.x = scale * pSrc->offset_x();
  if (pSrc->has_offset_y())
    offset.y = scale * pSrc->offset_y();

  dest = dp::FontDecl(ConvertColor(pSrc->color()), h);

  if (pSrc->has_stroke_color())
    dest.m_outlineColor = ConvertColor(pSrc->stroke_color());
}

void ConvertStyle(ShieldRuleProto const * pSrc, double scale, dp::FontDecl & dest)
{
  // fonts smaller than 8px look "jumpy" on LDPI devices
  uint8_t const h = max(8, static_cast<int>(pSrc->height() * scale));

  dest = dp::FontDecl(ConvertColor(pSrc->color()), h);

  if (pSrc->has_stroke_color())
    dest.m_outlineColor = ConvertColor(pSrc->stroke_color());
}

uint8_t GetFontSize(CaptionDefProto const * pSrc)
{
  return pSrc->height();
}

}
}
