#pragma once

#include "../base/base.hpp"
#include "../std/vector.hpp"

namespace search
{

struct CategoryInfo
{
  static uint32_t const DO_NOT_SUGGEST = 255;

  CategoryInfo() : m_prefixLengthToSuggest(DO_NOT_SUGGEST) {}

  vector<uint32_t> m_types;
  uint8_t m_prefixLengthToSuggest;
};

}  // namespace search
