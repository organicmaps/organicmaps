#pragma once

#include "base/string_utils.hpp"

#include <cstddef>
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
    Iterator & Move(UniChar c)
    {
      if (Rejects())
        return *this;

      if (Accepts())
      {
        auto currentIt = m_it;
        currentIt.Move(c);

        // While moving m_it, errors number decreases while matching unmatched symbols:
        // source:  a b c d e f
        // query:   a b c d e f
        // errors:  5 4 3 2 1 0
        //
        // After a misprinted symbol errors number remains the same:
        // source:  a b c d e f
        // query:   a b z d e f
        // errors:  5 4 3 3 2 1
        //
        // source:  a b c d e f
        // query:   a b d c e f
        // errors:  5 4 3 3 2 1
        //
        // source:  a b c d e f
        // query:   a b d e f
        // errors:  5 4 3 3 2
        //
        // source:  a b c d e f
        // query:   a b c z d e f
        // errors:  5 4 3 3 3 2 1
        //
        // Errors number cannot decrease after it has increased once.

        if (currentIt.ErrorsMade() > ErrorsMade())
          return *this;
      }

      m_it.Move(c);
      if (m_it.Accepts())
        m_accepts = true;

      return *this;
    }

    bool Accepts() const { return m_accepts; }
    bool Rejects() const { return !Accepts() && m_it.Rejects(); }
    size_t ErrorsMade() const { return m_it.ErrorsMade(); }
    size_t PrefixErrorsMade() const { return m_it.PrefixErrorsMade(); }

  private:
    friend class PrefixDFAModifier;

    Iterator(typename DFA::Iterator it) : m_it(it), m_accepts(m_it.Accepts()) {}

    typename DFA::Iterator m_it;
    bool m_accepts;
  };

  explicit PrefixDFAModifier(DFA && dfa) : m_dfa(std::move(dfa)) {}

  Iterator Begin() const { return Iterator(m_dfa.Begin()); }

private:
  DFA m_dfa;
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
