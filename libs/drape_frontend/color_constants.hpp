#pragma once

#include "drape/color.hpp"

#include <map>
#include <string>

namespace df
{
using ColorConstant = std::string;

inline std::string const kTransitColorPrefix = "transit_";
inline std::string const kTransitTextPrefix = "text_";
inline std::string const kTransitLinePrefix = "line_";

dp::Color GetColorConstant(ColorConstant const & constant);
std::map<std::string, dp::Color> const & GetTransitClearColors();
void LoadTransitColors();

ColorConstant GetTransitColorName(ColorConstant const & localName);
ColorConstant GetTransitTextColorName(ColorConstant const & localName);
}  //  namespace df
