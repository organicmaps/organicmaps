#pragma once

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

    bool Accepts() const { return !Rejects() && m_pos == m_s.size(); }
    bool Rejects() const { return m_rejected; }
    size_t ErrorsMade() const { return 0; }
    size_t PrefixErrorsMade() const { return 0; }

  private:
    friend class UniStringDFA;

    explicit Iterator(UniString const & s);

    UniString const & m_s;
    size_t m_pos;
    bool m_rejected;
  };

  explicit UniStringDFA(UniString const & s) : m_s(s) {}
  explicit UniStringDFA(std::string const & s);

  Iterator Begin() const { return Iterator(m_s); }

  friend std::string DebugPrint(UniStringDFA const & dfa);

private:
  UniString const m_s;
};
}  // namespace strings
