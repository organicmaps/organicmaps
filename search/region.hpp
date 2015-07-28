#pragma once

#include "std/string.hpp"
#include "std/vector.hpp"

namespace search
{
struct Region
{
  vector<size_t> m_ids;
  vector<size_t> m_matchedTokens;
  string m_enName;

  bool IsValid() const;

  void Swap(Region & rhs);

  bool operator<(Region const & rhs) const;
};

string DebugPrint(Region const & r);
}  // namespace search
