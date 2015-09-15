#pragma once

#include "drape_frontend/watch/pen_info.hpp"
#include "drape_frontend/watch/brush_info.hpp"
#include "drape_frontend/watch/icon_info.hpp"
#include "drape_frontend/watch/circle_info.hpp"

#include "drape/drape_global.hpp"

#include "geometry/point2d.hpp"

class LineDefProto;
class AreaRuleProto;
class SymbolRuleProto;
class CaptionDefProto;
class CircleRuleProto;
class ShieldRuleProto;

namespace df
{
namespace watch
{

dp::Color ConvertColor(uint32_t c);

void ConvertStyle(LineDefProto const * pSrc, double scale, PenInfo & dest);
void ConvertStyle(AreaRuleProto const * pSrc, BrushInfo & dest);
void ConvertStyle(SymbolRuleProto const * pSrc, IconInfo & dest);
void ConvertStyle(CircleRuleProto const * pSrc, double scale, CircleInfo & dest);
void ConvertStyle(CaptionDefProto const * pSrc, double scale, dp::FontDecl & dest, m2::PointD & offset);
void ConvertStyle(ShieldRuleProto const * pSrc, double scale, dp::FontDecl & dest);

uint8_t GetFontSize(CaptionDefProto const * pSrc);

}
}
