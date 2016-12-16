#pragma once

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <string>

namespace strings
{
class UniStringDFA
{
public:
  class Iterator
  {
  public:
    Iterator & Move(UniChar c);

    inline bool Accepts() const { return !Rejects() && m_pos == m_s.size(); }
    inline bool Rejects() const { return m_rejected; }

  private:
    friend class UniStringDFA;

    Iterator(UniString const & s);

    UniString const & m_s;
    size_t m_pos;
    bool m_rejected;
  };

  explicit UniStringDFA(UniString const & s);
  explicit UniStringDFA(std::string const & s);

  Iterator Begin() const;

private:
  UniString const m_s;
};
}  // namespace strings
