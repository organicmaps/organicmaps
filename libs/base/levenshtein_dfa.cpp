#include "base/levenshtein_dfa.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <iterator>
#include <queue>
#include <sstream>
#include <vector>

namespace strings
{
namespace
{
size_t AbsDiff(size_t a, size_t b)
{
  return a > b ? a - b : b - a;
}

class TransitionTable
{
public:
  TransitionTable(UniString const & s, std::vector<UniString> const & prefixMisprints, size_t prefixSize)
    : m_s(s)
    , m_size(s.size())
    , m_prefixMisprints(prefixMisprints)
    , m_prefixSize(prefixSize)
  {}

  void Move(LevenshteinDFA::State const & s, UniChar c, LevenshteinDFA::State & t)
  {
    t.Clear();
    for (auto const & p : s.m_positions)
      GetMoves(p, c, t);
    t.Normalize();
  }

private:
  void GetMoves(LevenshteinDFA::Position const & p, UniChar c, LevenshteinDFA::State & t)
  {
    auto & ps = t.m_positions;

    if (p.IsTransposed())
    {
      if (p.m_offset + 2 <= m_size && m_s[p.m_offset] == c)
        ps.emplace_back(p.m_offset + 2, p.m_errorsLeft, false /* transposed */);
      return;
    }

    ASSERT(p.IsStandard(), ());

    if (p.m_offset < m_size && m_s[p.m_offset] == c)
    {
      ps.emplace_back(p.m_offset + 1, p.m_errorsLeft, false /* transposed */);
      return;
    }

    if (p.m_errorsLeft == 0)
      return;

    ps.emplace_back(p.m_offset, p.m_errorsLeft - 1, false /* transposed */);

    if (p.m_offset < m_prefixSize)
    {
      // Allow only prefixMisprints for prefix.
      if (IsAllowedPrefixMisprint(c, p.m_offset))
        ps.emplace_back(p.m_offset + 1, p.m_errorsLeft - 1, false /* transposed */);
      return;
    }

    if (p.m_offset == m_size)
      return;

    ps.emplace_back(p.m_offset + 1, p.m_errorsLeft - 1, false /* transposed */);

    size_t i;
    if (FindRelevant(p, c, i))
    {
      ASSERT_GREATER(i, 0, (i));
      ASSERT_LESS_OR_EQUAL(p.m_offset + i + 1, m_size, ());
      ps.emplace_back(p.m_offset + i + 1, p.m_errorsLeft - i, false /* transposed */);

      if (i == 1)
        ps.emplace_back(p.m_offset, p.m_errorsLeft - 1, true /* transposed */);
    }
  }

  bool FindRelevant(LevenshteinDFA::Position const & p, UniChar c, size_t & i) const
  {
    size_t const limit = std::min(m_size - p.m_offset, p.m_errorsLeft + 1);

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
  size_t const m_size;
  std::vector<UniString> const m_prefixMisprints;
  size_t const m_prefixSize;
};
}  // namespace

// LevenshteinDFA ----------------------------------------------------------------------------------
// static
size_t const LevenshteinDFA::kStartingState = 0;
size_t const LevenshteinDFA::kRejectingState = 1;

// LevenshteinDFA::Position ------------------------------------------------------------------------
LevenshteinDFA::Position::Position(size_t offset, size_t errorsLeft, bool transposed)
  : m_offset(offset)
  , m_errorsLeft(errorsLeft)
  , m_transposed(transposed)
{}

bool LevenshteinDFA::Position::SubsumedBy(Position const & rhs) const
{
  if (m_errorsLeft >= rhs.m_errorsLeft)
    return false;

  auto const errorsAvail = rhs.m_errorsLeft - m_errorsLeft;

  if (IsStandard() && rhs.IsStandard())
    return AbsDiff(m_offset, rhs.m_offset) <= errorsAvail;

  if (IsStandard() && rhs.IsTransposed())
    return m_offset == rhs.m_offset && m_errorsLeft == 0;

  if (IsTransposed() && rhs.IsStandard())
    return AbsDiff(m_offset + 1, rhs.m_offset) <= errorsAvail;

  ASSERT(IsTransposed(), ());
  ASSERT(rhs.IsTransposed(), ());
  return m_offset == rhs.m_offset;
}

bool LevenshteinDFA::Position::operator<(Position const & rhs) const
{
  if (m_offset != rhs.m_offset)
    return m_offset < rhs.m_offset;
  if (m_errorsLeft != rhs.m_errorsLeft)
    return m_errorsLeft < rhs.m_errorsLeft;
  return m_transposed < rhs.m_transposed;
}

bool LevenshteinDFA::Position::operator==(Position const & rhs) const
{
  return m_offset == rhs.m_offset && m_errorsLeft == rhs.m_errorsLeft && m_transposed == rhs.m_transposed;
}

// LevenshteinDFA::State ---------------------------------------------------------------------------
void LevenshteinDFA::State::Normalize()
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
    {
      ++i;
    }
  }

  m_positions.erase(m_positions.begin() + j, m_positions.end());
  base::SortUnique(m_positions);
}

// LevenshteinDFA ----------------------------------------------------------------------------------
// static
LevenshteinDFA::LevenshteinDFA(UniString const & s, size_t prefixSize, std::vector<UniString> const & prefixMisprints,
                               size_t maxErrors)
  : m_size(s.size())
  , m_maxErrors(maxErrors)
{
  m_alphabet.assign(s.begin(), s.end());
  CHECK_LESS_OR_EQUAL(prefixSize, s.size(), ());

  auto const pSize = static_cast<std::iterator_traits<UniString::iterator>::difference_type>(prefixSize);
  for (auto it = s.begin(); std::distance(s.begin(), it) < pSize; ++it)
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

  std::queue<State> states;
  std::map<State, size_t> visited;

  auto pushState = [&states, &visited, this](State const & state, size_t id)
  {
    ASSERT_EQUAL(id, m_transitions.size(), ());
    ASSERT_EQUAL(visited.count(state), 0, (state, id));

    ASSERT_EQUAL(m_transitions.size(), m_accepting.size(), ());
    ASSERT_EQUAL(m_transitions.size(), m_errorsMade.size(), ());

    states.emplace(state);
    visited[state] = id;
    m_transitions.emplace_back(m_alphabet.size());
    m_accepting.push_back(false);
    m_errorsMade.push_back(ErrorsMade(state));
    m_prefixErrorsMade.push_back(PrefixErrorsMade(state));
  };

  pushState(MakeStart(), kStartingState);
  pushState(MakeRejecting(), kRejectingState);

  TransitionTable table(s, prefixMisprints, prefixSize);

  while (!states.empty())
  {
    auto const curr = states.front();
    states.pop();
    ASSERT(IsValid(curr), (curr));

    ASSERT_GREATER(visited.count(curr), 0, (curr));
    auto const id = visited[curr];
    ASSERT_LESS(id, m_transitions.size(), ());

    if (IsAccepting(curr))
      m_accepting[id] = true;

    for (size_t i = 0; i < m_alphabet.size(); ++i)
    {
      State next;
      table.Move(curr, m_alphabet[i], next);

      size_t nid;

      auto const it = visited.find(next);
      if (it == visited.end())
      {
        nid = visited.size();
        pushState(next, nid);
      }
      else
      {
        nid = it->second;
      }

      m_transitions[id][i] = nid;
    }
  }
}

LevenshteinDFA::LevenshteinDFA(std::string const & s, size_t prefixSize, size_t maxErrors)
  : LevenshteinDFA(MakeUniString(s), prefixSize, {} /* prefixMisprints */, maxErrors)
{}

LevenshteinDFA::LevenshteinDFA(UniString const & s, size_t maxErrors)
  : LevenshteinDFA(s, 0 /* prefixSize */, {} /* prefixMisprints */, maxErrors)
{}

LevenshteinDFA::LevenshteinDFA(std::string const & s, size_t maxErrors)
  : LevenshteinDFA(MakeUniString(s), 0 /* prefixSize */, {} /* prefixMisprints */, maxErrors)
{}

LevenshteinDFA::State LevenshteinDFA::MakeStart()
{
  State state;
  state.m_positions.emplace_back(0 /* offset */, m_maxErrors /* errorsLeft */, false /* transposed */);
  return state;
}

LevenshteinDFA::State LevenshteinDFA::MakeRejecting()
{
  return State();
}

bool LevenshteinDFA::IsValid(Position const & p) const
{
  return p.m_offset <= m_size && p.m_errorsLeft <= m_maxErrors;
}

bool LevenshteinDFA::IsValid(State const & s) const
{
  for (auto const & p : s.m_positions)
    if (!IsValid(p))
      return false;
  return true;
}

bool LevenshteinDFA::IsAccepting(Position const & p) const
{
  return p.IsStandard() && m_size - p.m_offset <= p.m_errorsLeft;
}

bool LevenshteinDFA::IsAccepting(State const & s) const
{
  for (auto const & p : s.m_positions)
    if (IsAccepting(p))
      return true;
  return false;
}

size_t LevenshteinDFA::ErrorsMade(State const & s) const
{
  size_t errorsMade = m_maxErrors;
  for (auto const & p : s.m_positions)
  {
    if (!IsAccepting(p))
      continue;
    auto const errorsLeft = p.m_errorsLeft - (m_size - p.m_offset);
    errorsMade = std::min(errorsMade, m_maxErrors - errorsLeft);
  }
  return errorsMade;
}

size_t LevenshteinDFA::PrefixErrorsMade(State const & s) const
{
  size_t errorsMade = m_maxErrors;
  for (auto const & p : s.m_positions)
    errorsMade = std::min(errorsMade, m_maxErrors - p.m_errorsLeft);
  return errorsMade;
}

size_t LevenshteinDFA::Move(size_t s, UniChar c) const
{
  ASSERT_GREATER(m_alphabet.size(), 0, ());
  ASSERT(is_sorted(m_alphabet.begin(), m_alphabet.end() - 1), ());

  size_t i;
  auto const it = lower_bound(m_alphabet.begin(), m_alphabet.end() - 1, c);
  if (it == m_alphabet.end() - 1 || *it != c)
    i = m_alphabet.size() - 1;
  else
    i = distance(m_alphabet.begin(), it);

  return m_transitions[s][i];
}

std::string DebugPrint(LevenshteinDFA::Position const & p)
{
  std::ostringstream os;
  os << "Position [" << p.m_offset << ", " << p.m_errorsLeft << ", " << p.m_transposed << "]";
  return os.str();
}

std::string DebugPrint(LevenshteinDFA::State const & s)
{
  std::ostringstream os;
  os << "State [";
  for (size_t i = 0; i < s.m_positions.size(); ++i)
  {
    os << DebugPrint(s.m_positions[i]);
    if (i + 1 != s.m_positions.size())
      os << ", ";
  }
  return os.str();
}
}  // namespace strings
