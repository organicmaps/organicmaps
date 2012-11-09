#pragma once

#include "../graphics/pen_info.hpp"
#include "../graphics/circle_info.hpp"
#include "../graphics/font_desc.hpp"


class LineDefProto;
class AreaRuleProto;
class SymbolRuleProto;
class CaptionDefProto;
class CircleRuleProto;


void ConvertStyle(LineDefProto const * pSrc, double scale, graphics::PenInfo & dest);
void ConvertStyle(AreaRuleProto const * pSrc, graphics::Color & dest);
void ConvertStyle(SymbolRuleProto const * pSrc, string & dest);
void ConvertStyle(CircleRuleProto const * pSrc, double scale, graphics::CircleInfo & dest);
void ConvertStyle(CaptionDefProto const * pSrc, double scale, graphics::FontDesc & dest);

uint8_t GetFontSize(CaptionDefProto const * pSrc);
