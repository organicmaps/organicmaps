#pragma once

#include "graphics/pen.hpp"
#include "graphics/brush.hpp"
#include "graphics/icon.hpp"
#include "graphics/circle.hpp"
#include "graphics/font_desc.hpp"

class LineDefProto;
class AreaRuleProto;
class SymbolRuleProto;
class CaptionDefProto;
class CircleRuleProto;

graphics::Color ConvertColor(uint32_t c);

void ConvertStyle(LineDefProto const * pSrc, double scale, graphics::Pen::Info & dest);
void ConvertStyle(AreaRuleProto const * pSrc, graphics::Brush::Info & dest);
void ConvertStyle(SymbolRuleProto const * pSrc, graphics::Icon::Info & dest);
void ConvertStyle(CircleRuleProto const * pSrc, double scale, graphics::Circle::Info & dest);
void ConvertStyle(CaptionDefProto const * pSrc, double scale, graphics::FontDesc & dest, m2::PointD & offset);

uint8_t GetFontSize(CaptionDefProto const * pSrc);
