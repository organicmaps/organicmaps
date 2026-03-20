#include "base/levenshtein_dfa.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <unordered_map>
#include <vector>

namespace strings
{
namespace
{

struct Position
{
  Position() = default;
  Position(size_t offset, uint8_t errorsLeft, bool transposed)
    : m_offset(offset)
    , m_errorsLeft(errorsLeft)
    , m_transposed(transposed)
  {}

  // Returns true when |rhs| dominates this position, making it safe to remove
  // this position from the state.
  bool SubsumedBy(Position const & rhs) const
  {
    if (m_errorsLeft >= rhs.m_errorsLeft)
      return false;

    uint8_t const errorsAvail = rhs.m_errorsLeft - m_errorsLeft;

    if (!m_transposed && !rhs.m_transposed)
      return math::AbsDiff(m_offset, rhs.m_offset) <= errorsAvail;

    if (!m_transposed && rhs.m_transposed)
      return m_offset == rhs.m_offset && m_errorsLeft == 0;

    if (m_transposed && !rhs.m_transposed)
      return math::AbsDiff(m_offset + 1, rhs.m_offset) <= errorsAvail;

    // Both transposed.
    return m_offset == rhs.m_offset;
  }

  bool operator<(Position const & rhs) const
  {
    if (m_offset != rhs.m_offset)
      return m_offset < rhs.m_offset;
    if (m_errorsLeft != rhs.m_errorsLeft)
      return m_errorsLeft < rhs.m_errorsLeft;
    return m_transposed < rhs.m_transposed;
  }

  bool operator==(Position const & rhs) const
  {
    return m_offset == rhs.m_offset && m_errorsLeft == rhs.m_errorsLeft && m_transposed == rhs.m_transposed;
  }

  bool IsAccepting(size_t strSize) const
  {
    ASSERT_GREATER_OR_EQUAL(strSize, m_offset, ());
    return !m_transposed && strSize - m_offset <= m_errorsLeft;
  }

  size_t m_offset = 0;
  uint8_t m_errorsLeft = 0;
  bool m_transposed = false;
};

uint8_t SubtractErrors(uint8_t err, size_t toSubtract)
{
  ASSERT_GREATER_OR_EQUAL(err, toSubtract, ());
  return static_cast<uint8_t>(size_t(err) - toSubtract);
}

struct State
{
  void Normalize()
  {
    size_t i = 0;
    size_t j = m_positions.size();

    while (i < j)
    {
      auto const & cur = m_positions[i];

      auto it = find_if(m_positions.begin(), m_positions.begin() + j,
                        [&](Position const & rhs) { return cur.SubsumedBy(rhs); });
      if (it != m_positions.begin() + j)
      {
        ASSERT_GREATER(j, 0, ());
        --j;
        std::swap(m_positions[i], m_positions[j]);
      }
      else
        ++i;
    }

    m_positions.erase(m_positions.begin() + j, m_positions.end());
    base::SortUnique(m_positions);
  }

  void Clear() { m_positions.clear(); }

  bool IsAccepting(size_t strSize) const
  {
    for (auto const & p : m_positions)
      if (p.IsAccepting(strSize))
        return true;
    return false;
  }

  uint8_t ErrorsMade(size_t strSize, uint8_t maxErrors) const
  {
    uint8_t result = maxErrors;
    for (auto const & p : m_positions)
    {
      if (!p.IsAccepting(strSize))
        continue;
      uint8_t const errorsLeft = SubtractErrors(p.m_errorsLeft, strSize - p.m_offset);
      result = std::min(result, SubtractErrors(maxErrors, errorsLeft));
    }
    return result;
  }

  uint8_t PrefixErrorsMade(uint8_t maxErrors) const
  {
    uint8_t result = maxErrors;
    for (auto const & p : m_positions)
      result = std::min(result, SubtractErrors(maxErrors, p.m_errorsLeft));
    return result;
  }

  std::vector<Position> m_positions;
};

// Compact stack-allocated key for State used in the visited hash map.
// Each Position is encoded as a single uint32_t:
//   bits [0]     : transposed
//   bits [3:1]   : errorsLeft  (supports up to 7)
//   bits [31:4]  : offset      (supports strings up to 2^28 chars)
//
// Max positions after normalization with maxErrors=2: (2*2+1)*(2+1) = 15.
// We use 24 as a safe upper bound.
static constexpr size_t kMaxStatePositions = 24;

struct StateKey
{
  // [0]=count, rest=encoded positions
  std::array<uint32_t, kMaxStatePositions + 1> data;  // zero initialization is not needed
  bool operator==(StateKey const & rhs) const
  {
    if (data[0] != rhs.data[0])
      return false;
    return std::equal(data.begin() + 1, data.begin() + 1 + data[0], rhs.data.begin() + 1);
  }
};

StateKey MakeStateKey(State const & s)
{
  ASSERT_LESS_OR_EQUAL(s.m_positions.size(), kMaxStatePositions, ());
  StateKey key;
  key.data[0] = static_cast<uint32_t>(s.m_positions.size());
  for (size_t i = 0; i < s.m_positions.size(); ++i)
  {
    auto const & p = s.m_positions[i];
    key.data[i + 1] = (static_cast<uint32_t>(p.m_offset) << 4) | (static_cast<uint32_t>(p.m_errorsLeft) << 1) |
                      (p.m_transposed ? 1u : 0u);
  }
  return key;
}

struct StateKeyHash
{
  size_t operator()(StateKey const & k) const noexcept
  {
    // Polynomial hash over the count and encoded positions only.
    size_t h = k.data[0];
    for (size_t i = 1; i <= k.data[0]; ++i)
      h = h * 2654435761u + k.data[i];
    return h;
  }
};

class TransitionTable
{
public:
  TransitionTable(UniString const & s, std::vector<UniString> const & prefixMisprints, size_t prefixSize)
    : m_s(s)
    , m_prefixMisprints(prefixMisprints)
    , m_prefixSize(prefixSize)
  {}

  void Move(State const & s, UniChar c, State & t)
  {
    t.Clear();
    for (auto const & p : s.m_positions)
      GetMoves(p, c, t);
    t.Normalize();
  }

private:
  void GetMoves(Position const & p, UniChar c, State & t)
  {
    auto & ps = t.m_positions;
    size_t const size = m_s.size();

    if (p.m_transposed)
    {
      if (p.m_offset + 2 <= size && m_s[p.m_offset] == c)
        ps.emplace_back(p.m_offset + 2, p.m_errorsLeft, false /* transposed */);
      return;
    }

    if (p.m_offset < size && m_s[p.m_offset] == c)
    {
      ps.emplace_back(p.m_offset + 1, p.m_errorsLeft, false /* transposed */);
      return;
    }

    if (p.m_errorsLeft == 0)
      return;

    uint8_t const errLeft1 = p.m_errorsLeft - 1;
    ps.emplace_back(p.m_offset, errLeft1, false /* transposed */);

    if (p.m_offset < m_prefixSize)
    {
      // Allow only prefixMisprints for prefix.
      if (IsAllowedPrefixMisprint(c, p.m_offset))
        ps.emplace_back(p.m_offset + 1, errLeft1, false /* transposed */);
      return;
    }

    if (p.m_offset == size)
      return;

    ps.emplace_back(p.m_offset + 1, errLeft1, false /* transposed */);

    size_t i;
    if (FindRelevant(p, c, i))
    {
      ASSERT_GREATER(i, 0, ());
      ASSERT_LESS(p.m_offset + i, size, ());
      ps.emplace_back(p.m_offset + i + 1, SubtractErrors(p.m_errorsLeft, i), false /* transposed */);

      if (i == 1)
        ps.emplace_back(p.m_offset, errLeft1, true /* transposed */);
    }
  }

  bool FindRelevant(Position const & p, UniChar c, size_t & i) const
  {
    size_t const limit = std::min(m_s.size() - p.m_offset, size_t{p.m_errorsLeft} + 1);
    for (i = 0; i < limit; ++i)
      if (m_s[p.m_offset + i] == c)
        return true;
    return false;
  }

  bool IsAllowedPrefixMisprint(UniChar c, size_t position) const
  {
    CHECK_LESS(position, m_prefixSize, ());
    for (auto const & misprints : m_prefixMisprints)
      if (base::IsExist(misprints, c) && base::IsExist(misprints, m_s[position]))
        return true;
    return false;
  }

  UniString const & m_s;
  std::vector<UniString> const & m_prefixMisprints;
  size_t const m_prefixSize;
};

}  // namespace

// LevenshteinDFA ----------------------------------------------------------------------------------
LevenshteinDFA::LevenshteinDFA(UniString const & s, size_t prefixSize, std::vector<UniString> const & prefixMisprints,
                               uint8_t maxErrors)
{
  m_alphabet.assign(s.begin(), s.end());
  ASSERT_LESS_OR_EQUAL(prefixSize, s.size(), ());

  for (auto it = s.begin(); std::distance(s.begin(), it) < ptrdiff_t(prefixSize); ++it)
  {
    for (auto const & misprints : prefixMisprints)
      if (base::IsExist(misprints, *it))
        m_alphabet.insert(m_alphabet.end(), misprints.begin(), misprints.end());
  }
  base::SortUnique(m_alphabet);

  UniChar missed = 0;
  for (size_t i = 0; i < m_alphabet.size() && missed >= m_alphabet[i]; ++i)
    if (missed == m_alphabet[i])
      ++missed;
  m_alphabet.push_back(missed);

  size_t const alphabetSize = m_alphabet.size();
  size_t const strSize = s.size();

  // statesById stores each DFA State by its numeric ID. The BFS loop iterates
  // over statesById by index so that newly discovered states (appended at the end)
  // are automatically visited without a separate queue.
  std::vector<State> statesById;
  std::unordered_map<StateKey, uint32_t, StateKeyHash> visited;

  size_t constexpr kReserveSize = 128;
  statesById.reserve(kReserveSize);
  visited.reserve(kReserveSize);
  m_transitions.reserve(kReserveSize * alphabetSize);
  m_accepting.reserve(kReserveSize);
  m_errorsMade.reserve(kReserveSize);
  m_prefixErrorsMade.reserve(kReserveSize);

  // Registers a new state, id = statesById.size().
  auto const addState = [&](State state)
  {
    m_transitions.insert(m_transitions.end(), alphabetSize, 0);
    m_accepting.push_back(state.IsAccepting(strSize));
    m_errorsMade.push_back(state.ErrorsMade(strSize, maxErrors));
    m_prefixErrorsMade.push_back(state.PrefixErrorsMade(maxErrors));
    statesById.push_back(std::move(state));
  };
  auto const addVisitedAndState = [&](State state)
  {
    visited[MakeStateKey(state)] = statesById.size();
    addState(std::move(state));
  };

  // kStartingState = 0
  {
    State start;
    start.m_positions.emplace_back(0 /* offset */, maxErrors /* errorsLeft */, false /* transposed */);
    addVisitedAndState(std::move(start));
  }
  // kRejectingState = 1
  addVisitedAndState(State{});

  TransitionTable table(s, prefixMisprints, prefixSize);

  State next;
  for (size_t id = 0; id < statesById.size(); ++id)
    for (size_t i = 0; i < alphabetSize; ++i)
    {
      table.Move(statesById[id], m_alphabet[i], next);

      auto const res = visited.emplace(MakeStateKey(next), base::asserted_cast<uint32_t>(statesById.size()));
      m_transitions[id * alphabetSize + i] = res.first->second;

      // Note that statesById is growing in addState.
      if (res.second)
        addState(std::move(next));
    }
}

LevenshteinDFA::LevenshteinDFA(std::string const & s, size_t prefixSize, uint8_t maxErrors)
  : LevenshteinDFA(MakeUniString(s), prefixSize, {} /* prefixMisprints */, maxErrors)
{}

LevenshteinDFA::LevenshteinDFA(UniString const & s, uint8_t maxErrors)
  : LevenshteinDFA(s, 0 /* prefixSize */, {} /* prefixMisprints */, maxErrors)
{}

LevenshteinDFA::LevenshteinDFA(std::string const & s, uint8_t maxErrors)
  : LevenshteinDFA(MakeUniString(s), 0 /* prefixSize */, {} /* prefixMisprints */, maxErrors)
{}

size_t LevenshteinDFA::Move(size_t s, UniChar c) const
{
  ASSERT_GREATER(m_alphabet.size(), 0, ());
  ASSERT(is_sorted(m_alphabet.begin(), m_alphabet.end() - 1), ());

  auto const it = lower_bound(m_alphabet.begin(), m_alphabet.end() - 1, c);
  size_t const i = (it == m_alphabet.end() - 1 || *it != c) ? m_alphabet.size() - 1
                                                            : static_cast<size_t>(distance(m_alphabet.begin(), it));

  return m_transitions[s * m_alphabet.size() + i];
}
}  // namespace strings
