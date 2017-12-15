#pragma once

#include "base/string_utils.hpp"

#include <cstdint>
#include <map>

namespace search
{
class IdfMap
{
public:
  struct Delegate
  {
    virtual ~Delegate() = default;

    virtual uint64_t GetNumDocs(strings::UniString const & token, bool isPrefix) const = 0;
  };

  IdfMap(Delegate & delegate, double unknownIdf);

  void Set(strings::UniString const & s, bool isPrefix, double idf)
  {
    SetImpl(isPrefix ? m_prefixIdfs : m_fullIdfs, s, idf);
  }

  double Get(strings::UniString const & s, bool isPrefix)
  {
    return GetImpl(isPrefix ? m_prefixIdfs : m_fullIdfs, s, isPrefix);
  }

private:
  using Map = std::map<strings::UniString, double>;

  void SetImpl(Map & idfs, strings::UniString const & s, double idf) { idfs[s] = idf; }
  double GetImpl(Map & idfs, strings::UniString const & s, bool isPrefix);

  Map m_fullIdfs;
  Map m_prefixIdfs;

  Delegate & m_delegate;
  double m_unknownIdf;
};
}  // namespace search
