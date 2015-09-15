#pragma once

#include "drape/drape_global.hpp"

namespace df
{
namespace watch
{

struct BrushInfo
{
  dp::Color m_color;

  BrushInfo() = default;
  explicit BrushInfo(dp::Color const & color) : m_color(color) {}
};

}
}
