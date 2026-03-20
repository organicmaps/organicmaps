#pragma once

#include "base/string_utils.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace strings
{
// This class represents a DFA recognizing a language consisting of
// all words that are close enough to a given string, in terms of
// Levenshtein distance. Levenshtein distance treats deletions,
// insertions and replacements as errors. Transpositions are handled
// in a quite special way - its assumed that all transformations are
// applied in parallel, i.e. the Levenshtein distance between "ab" and
// "bca" is three, not two.  The code is based on the work "Fast
// String Correction with Levenshtein-Automata" by Klaus U. Schulz and
// Stoyan Mihov.  For a fixed number of allowed errors and fixed
// alphabet the construction time and size of automata is
// O(length of the pattern), but the size grows exponentially with the
// number of errors, so be reasonable and don't use this class when
// the number of errors is too high.
//
// *NOTE* The class *IS* thread-safe.
//
// TODO (@y): consider to implement a factory of automata, that will
// be able to construct them quickly for a fixed number of errors.
class LevenshteinDFA
{
public:
  static size_t constexpr kStartingState = 0;
  static size_t constexpr kRejectingState = 1;

  // An iterator to the current state in the DFA.
  //
  // *NOTE* The class *IS NOT* thread safe. Moreover, it should not be
  // used after destruction of the corresponding DFA.
  class Iterator
  {
  public:
    Iterator & Move(UniChar c)
    {
      m_s = m_dfa.Move(m_s, c);
      return *this;
    }

    bool Accepts() const { return m_dfa.IsAccepting(m_s); }
    bool Rejects() const { return m_dfa.IsRejecting(m_s); }

    size_t ErrorsMade() const { return m_dfa.ErrorsMade(m_s); }
    size_t PrefixErrorsMade() const { return m_dfa.PrefixErrorsMade(m_s); }

  private:
    friend class LevenshteinDFA;

    explicit Iterator(LevenshteinDFA const & dfa) : m_s(kStartingState), m_dfa(dfa) {}

    size_t m_s;
    LevenshteinDFA const & m_dfa;
  };

  LevenshteinDFA() = default;
  LevenshteinDFA(LevenshteinDFA const &) = delete;
  LevenshteinDFA(LevenshteinDFA &&) = default;
  LevenshteinDFA & operator=(LevenshteinDFA &&) = default;

  LevenshteinDFA(UniString const & s, size_t prefixSize, std::vector<UniString> const & prefixMisprints,
                 uint8_t maxErrors);
  LevenshteinDFA(std::string const & s, size_t prefixSize, uint8_t maxErrors);
  LevenshteinDFA(UniString const & s, uint8_t maxErrors);
  LevenshteinDFA(std::string const & s, uint8_t maxErrors);

  bool IsEmpty() const { return m_alphabet.empty(); }

  Iterator Begin() const { return Iterator(*this); }

  size_t GetNumStates() const { return m_alphabet.empty() ? 0 : m_transitions.size() / m_alphabet.size(); }
  size_t GetAlphabetSize() const { return m_alphabet.size(); }

private:
  friend class Iterator;

  bool IsAccepting(size_t s) const { return m_accepting[s]; }
  bool IsRejecting(size_t s) const { return s == kRejectingState; }
  size_t ErrorsMade(size_t s) const { return m_errorsMade[s]; }
  size_t PrefixErrorsMade(size_t s) const { return m_prefixErrorsMade[s]; }

  size_t Move(size_t s, UniChar c) const;

  std::vector<UniChar> m_alphabet;

  // Flat transition table: m_transitions[state * alphabetSize + charIndex] = nextState.
  std::vector<uint32_t> m_transitions;
  std::vector<bool> m_accepting;
  std::vector<uint8_t> m_errorsMade;
  std::vector<uint8_t> m_prefixErrorsMade;
};
}  // namespace strings
