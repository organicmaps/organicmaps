#pragma once

#include "software_renderer/pen_info.hpp"
#include "software_renderer/brush_info.hpp"
#include "software_renderer/icon_info.hpp"
#include "software_renderer/circle_info.hpp"

#include "drape/drape_global.hpp"

#include "geometry/point2d.hpp"

class LineDefProto;
class AreaRuleProto;
class SymbolRuleProto;
class CaptionDefProto;
class ShieldRuleProto;

namespace software_renderer
{

dp::Color ConvertColor(uint32_t c);

void ConvertStyle(LineDefProto const * pSrc, double scale, PenInfo & dest);
void ConvertStyle(AreaRuleProto const * pSrc, BrushInfo & dest);
void ConvertStyle(SymbolRuleProto const * pSrc, IconInfo & dest);
void ConvertStyle(CaptionDefProto const * pSrc, double scale, dp::FontDecl & dest, m2::PointD & offset);
void ConvertStyle(ShieldRuleProto const * pSrc, double scale, dp::FontDecl & dest);

uint8_t GetFontSize(CaptionDefProto const * pSrc);

}
