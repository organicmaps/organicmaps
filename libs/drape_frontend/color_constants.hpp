#pragma once

#include "drape/color.hpp"

#include <map>
#include <string>

namespace df
{
using ColorConstant = std::string_view;

std::string constexpr kTransitColorPrefix = "transit_";
std::string constexpr kTransitTextPrefix = "text_";
std::string constexpr kTransitLinePrefix = "line_";

dp::Color GetColorConstant(ColorConstant const & constant);

using ColorsMapT = std::map<std::string, dp::Color, std::less<>>;
ColorsMapT const & GetTransitClearColors();
void LoadTransitColors();

std::string GetTransitColorName(ColorConstant const & localName);
std::string GetTransitTextColorName(ColorConstant const & localName);
}  //  namespace df
