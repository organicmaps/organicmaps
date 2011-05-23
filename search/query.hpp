#pragma once

#include "../base/base.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"

namespace search1
{

class Query
{
public:
  explicit Query(string const & query);
private:
  vector<string> m_keywords;
  string m_prefix;
};

}  // namespace search1
