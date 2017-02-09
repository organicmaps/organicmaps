#pragma once

#include "drape/color.hpp"

#include <string>

namespace df
{
using ColorConstant = std::string;

dp::Color GetColorConstant(ColorConstant const & constant);
} //  namespace df
