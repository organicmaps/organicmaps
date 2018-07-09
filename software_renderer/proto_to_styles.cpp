#include "proto_to_styles.hpp"

#include "indexer/drules_include.hpp"

#include "std/algorithm.hpp"
#include "std/vector.hpp"

namespace
{

double ConvertWidth(double w, double scale)
{
  return max(w * scale, 1.0);
}

}

namespace software_renderer
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

    if (dd.offset() != 0)
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

    if (ps.offset() != 0)
      dest.m_offset = ps.offset() * scale;
  }

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

void ConvertStyle(AreaRuleProto const * pSrc, BrushInfo & dest)
{
  dest.m_color = ConvertColor(pSrc->color());
}

void ConvertStyle(SymbolRuleProto const * pSrc, IconInfo & dest)
{
  dest.m_name = pSrc->name();
}

void ConvertStyle(CaptionDefProto const * pSrc, double scale, dp::FontDecl & dest, m2::PointD & offset)
{
  // fonts smaller than 8px look "jumpy" on LDPI devices
  uint8_t const h = max(8, static_cast<int>(pSrc->height() * scale));

  offset = m2::PointD(0, 0);
  if (pSrc->offset_x() != 0)
    offset.x = scale * pSrc->offset_x();
  if (pSrc->offset_y() != 0)
    offset.y = scale * pSrc->offset_y();

  dest = dp::FontDecl(ConvertColor(pSrc->color()), h);

  if (pSrc->stroke_color() != 0)
    dest.m_outlineColor = ConvertColor(pSrc->stroke_color());
}

void ConvertStyle(ShieldRuleProto const * pSrc, double scale, dp::FontDecl & dest)
{
  // fonts smaller than 8px look "jumpy" on LDPI devices
  uint8_t const h = max(8, static_cast<int>(pSrc->height() * scale));

  dest = dp::FontDecl(ConvertColor(pSrc->color()), h);

  if (pSrc->stroke_color() != 0)
    dest.m_outlineColor = ConvertColor(pSrc->stroke_color());
}

uint8_t GetFontSize(CaptionDefProto const * pSrc)
{
  return pSrc->height();
}

}
