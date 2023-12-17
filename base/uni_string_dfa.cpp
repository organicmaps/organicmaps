#include "base/uni_string_dfa.hpp"

#include "base/assert.hpp"

namespace strings
{
// UniStringDFA::Iterator --------------------------------------------------------------------------
UniStringDFA::Iterator::Iterator(UniString const & s) : m_s(s), m_pos(0), m_rejected(false) {}

UniStringDFA::Iterator & UniStringDFA::Iterator::Move(UniChar c)
{
  if (Accepts())
  {
    m_rejected = true;
    return *this;
  }

  if (Rejects())
    return *this;

  ASSERT_LESS(m_pos, m_s.size(), ());
  if (m_s[m_pos] != c)
  {
    m_rejected = true;
    return *this;
  }

  ++m_pos;
  return *this;
}

// UniStringDFA::UniStringDFA ----------------------------------------------------------------------
UniStringDFA::UniStringDFA(std::string const & s) : UniStringDFA(MakeUniString(s)) {}

std::string DebugPrint(UniStringDFA const & dfa)
{
  return DebugPrint(dfa.m_s);
}

}  // namespace strings
