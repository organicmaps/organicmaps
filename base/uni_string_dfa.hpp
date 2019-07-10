#pragma once

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <cstddef>
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

    bool Accepts() const { return !Rejects() && m_pos == m_s.size(); }
    bool Rejects() const { return m_rejected; }
    size_t ErrorsMade() const { return 0; }
    size_t PrefixErrorsMade() const { return 0; }

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
