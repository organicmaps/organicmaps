#pragma once

#include "drape/color.hpp"

#include <map>
#include <string>
#include <string_view>

namespace df
{
using ColorConstant = std::string_view;

std::string_view constexpr kTransitLineColorPrefix = "transit_line_";
std::string_view constexpr kTransitTextColorPrefix = "transit_text_";

dp::Color GetColorConstant(ColorConstant const & constant);

using ColorsMapT = std::map<std::string, dp::Color, std::less<>>;
ColorsMapT const & GetTransitClearColors();
void LoadTransitColors();

std::string GetTransitColorName(ColorConstant const & localName);
std::string GetTransitTextColorName(ColorConstant const & localName);
}  //  namespace df
