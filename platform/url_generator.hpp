#pragma once

#include "../base/pseudo_random.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"

class UrlGenerator
{
  LCG32 m_randomGenerator;
  vector<string> m_firstGroup;
  vector<string> m_secondGroup;

public:
  UrlGenerator();
  explicit UrlGenerator(vector<string> const & firstGroup, vector<string> const & secondGroup);
  /// @return Always return empty string if all urls were already popped
  string PopNextUrl();
};
