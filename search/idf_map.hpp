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

    virtual uint64_t GetNumDocs(strings::UniString const & token) const = 0;
  };

  IdfMap(Delegate & delegate, double unknownIdf);

  void Set(strings::UniString const & s, double idf) { m_idfs[s] = idf; }
  double Get(strings::UniString const & s);

private:
  std::map<strings::UniString, double> m_idfs;

  Delegate & m_delegate;
  double m_unknownIdf;
};
}  // namespace search
