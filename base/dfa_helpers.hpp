#pragma once

#include "base/string_utils.hpp"

#include <string>

namespace strings
{
template <typename DFA>
class PrefixDFAModifier
{
public:
  class Iterator
  {
  public:
    Iterator & Move(strings::UniChar c)
    {
      if (Accepts() || Rejects())
        return *this;

      m_it.Move(c);
      if (m_it.Accepts())
        m_accepts = true;

      return *this;
    }

    bool Accepts() const { return m_accepts; }
    bool Rejects() const { return !Accepts() && m_it.Rejects(); }

  private:
    friend class PrefixDFAModifier;

    Iterator(typename DFA::Iterator it) : m_it(it), m_accepts(m_it.Accepts()) {}

    typename DFA::Iterator m_it;
    bool m_accepts;
  };

  explicit PrefixDFAModifier(DFA const & dfa) : m_dfa(dfa) {}

  Iterator Begin() const { return Iterator(m_dfa.Begin()); }

private:
  DFA const m_dfa;
};

template <typename DFAIt, typename It>
void DFAMove(DFAIt & it, It begin, It end)
{
  for (; begin != end; ++begin)
    it.Move(*begin);
}

template <typename DFAIt>
void DFAMove(DFAIt & it, UniString const & s)
{
  DFAMove(it, s.begin(), s.end());
}

template <typename DFAIt>
void DFAMove(DFAIt & it, std::string const & s)
{
  DFAMove(it, MakeUniString(s));
}
}  // namespace strings
