#pragma once

#include "drape/color.hpp"

#include <string>

namespace df
{
using ColorConstant = std::string;

dp::Color GetColorConstant(ColorConstant const & constant);

void LoadTransitColors();

ColorConstant GetTransitColorName(ColorConstant const & localName);
ColorConstant GetTransitTextColorName(ColorConstant const & localName);
} //  namespace df
