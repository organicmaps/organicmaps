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

  IdfMap(Delegate const & delegate, double unknownIdf);

  double Get(strings::UniString const & s, bool isPrefix)
  {
    return GetImpl(isPrefix ? m_prefixIdfs : m_fullIdfs, s, isPrefix);
  }

private:
  using Map = std::map<strings::UniString, double>;

  double GetImpl(Map & idfs, strings::UniString const & s, bool isPrefix);

  Map m_fullIdfs;
  Map m_prefixIdfs;

  Delegate const & m_delegate;
  double m_unknownIdf;
};
}  // namespace search
