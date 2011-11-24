#pragma once

#include "../yg/pen_info.hpp"
#include "../yg/circle_info.hpp"
#include "../yg/font_desc.hpp"


class LineDefProto;
class AreaRuleProto;
class SymbolRuleProto;
class CaptionDefProto;
class CircleRuleProto;


void ConvertStyle(LineDefProto const * pSrc, double scale, yg::PenInfo & dest);
void ConvertStyle(AreaRuleProto const * pSrc, yg::Color & dest);
void ConvertStyle(SymbolRuleProto const * pSrc, string & dest);
void ConvertStyle(CircleRuleProto const * pSrc, double scale, yg::CircleInfo & dest);
void ConvertStyle(CaptionDefProto const * pSrc, double scale, yg::FontDesc & dest);

uint8_t GetFontSize(CaptionDefProto const * pSrc);
