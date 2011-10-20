#include "search_string_utils.hpp"

strings::UniString search::FeatureTypeToString(uint32_t type)
{
  string s = "!type:" + strings::to_string(type);
  return strings::UniString(s.begin(), s.end());
}
