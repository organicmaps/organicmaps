#pragma once

#include "base/string_utils.hpp"

#include <map>

namespace search
{
class IdfMap
{
public:
  explicit IdfMap(double unknownIdf);

  void Set(strings::UniString const & s, double idf) { m_idfs[s] = idf; }
  double Get(strings::UniString const & s) const;

private:
  std::map<strings::UniString, double> m_idfs;
  double m_unknownIdf;
};
}  // namespace search
