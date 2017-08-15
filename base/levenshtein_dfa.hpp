#pragma once

#include "base/string_utils.hpp"

#include <cstddef>
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
  static size_t const kStartingState;
  static size_t const kRejectingState;
  
  struct Position
  {
    Position() = default;
    Position(size_t offset, size_t errorsLeft, bool transposed);

    // SubsumedBy is a relation on two positions, which allows to
    // efficiently remove unnecessary positions in a state. When the
    // function returns true, it means that |rhs| is more powerful
    // than the current position and it is safe to remove the current
    // position from the state, if the state contains |rhs|.
    bool SubsumedBy(Position const & rhs) const;

    bool operator<(Position const & rhs) const;
    bool operator==(Position const & rhs) const;

    inline bool IsStandard() const { return !m_transposed; }
    inline bool IsTransposed() const { return m_transposed; }

    size_t m_offset = 0;
    size_t m_errorsLeft = 0;
    bool m_transposed = false;
  };

  struct State
  {
    void Normalize();
    inline void Clear() { m_positions.clear(); }

    inline bool operator<(State const & rhs) const { return m_positions < rhs.m_positions; }

    std::vector<Position> m_positions;
  };

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

  private:
    friend class LevenshteinDFA;

    explicit Iterator(LevenshteinDFA const & dfa) : m_s(kStartingState), m_dfa(dfa) {}

    size_t m_s;
    LevenshteinDFA const & m_dfa;
  };

  LevenshteinDFA(UniString const & s, size_t prefixCharsToKeep, size_t maxErrors);
  LevenshteinDFA(std::string const & s, size_t prefixCharsToKeep, size_t maxErrors);
  LevenshteinDFA(UniString const & s, size_t maxErrors);
  LevenshteinDFA(std::string const & s, size_t maxErrors);

  inline Iterator Begin() const { return Iterator(*this); }

  size_t GetNumStates() const { return m_transitions.size(); }
  size_t GetAlphabetSize() const { return m_alphabet.size(); }

private:
  friend class Iterator;

  State MakeStart();
  State MakeRejecting();

  bool IsValid(Position const & p) const;
  bool IsValid(State const & s) const;

  bool IsAccepting(Position const & p) const;
  bool IsAccepting(State const & s) const;
  inline bool IsAccepting(size_t s) const { return m_accepting[s]; }

  inline bool IsRejecting(State const & s) const { return s.m_positions.empty(); }
  inline bool IsRejecting(size_t s) const { return s == kRejectingState; }

  size_t Move(size_t s, UniChar c) const;

  size_t const m_size;
  size_t const m_maxErrors;

  std::vector<UniChar> m_alphabet;

  std::vector<std::vector<size_t>> m_transitions;
  std::vector<bool> m_accepting;
};

std::string DebugPrint(LevenshteinDFA::Position const & p);
std::string DebugPrint(LevenshteinDFA::State const & s);
}  // namespace strings
