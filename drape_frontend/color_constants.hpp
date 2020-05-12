#pragma once

#include "drape/color.hpp"

#include <map>
#include <string>

namespace df
{
using ColorConstant = std::string;

dp::Color GetColorConstant(ColorConstant const & constant);
std::map<std::string, dp::Color> const & GetClearColors();
void LoadTransitColors();

ColorConstant GetTransitColorName(ColorConstant const & localName);
ColorConstant GetTransitTextColorName(ColorConstant const & localName);
} //  namespace df
