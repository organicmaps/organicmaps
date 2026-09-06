#pragma once

#include "drape/color.hpp"

#include <map>
#include <string>

namespace df
{
using ColorConstant = std::string_view;

inline std::string const kTransitColorPrefix = "transit_";
inline std::string const kTransitTextPrefix = "text_";
// constexpr doesn't compile on Android now
inline std::string const kTransitLineColorPrefix = kTransitColorPrefix + "line_";
std::string const kTransitTextColorPrefix = kTransitColorPrefix + kTransitTextPrefix;

dp::Color GetColorConstant(ColorConstant const & constant);

using ColorsMapT = std::map<std::string, dp::Color, std::less<>>;
ColorsMapT const & GetTransitClearColors();
void LoadTransitColors();

std::string GetTransitColorName(ColorConstant const & localName);
std::string GetTransitTextColorName(ColorConstant const & localName);
}  //  namespace df
