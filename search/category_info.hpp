#pragma once

#include "../base/base.hpp"
#include "../std/vector.hpp"

namespace search
{

struct CategoryInfo
{
  static uint32_t const DO_NOT_SUGGEST = 255;

  CategoryInfo() : m_score(DO_NOT_SUGGEST) {}

  vector<uint32_t> m_types;
  // Score to
  uint8_t m_score;
};

}  // namespace search
