#include "search/tracer.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

using namespace std;

namespace search
{
// Tracer::Parse -----------------------------------------------------------------------------------
Tracer::Parse::Parse(vector<TokenType> const & types, bool category) : m_category(category)
{
  size_t i = 0;
  while (i != types.size())
  {
    auto const type = types[i];
    auto j = i + 1;
    while (j != types.size() && types[j] == type)
      ++j;
    m_ranges[type] = TokenRange(i, j);
    i = j;
  }
}

Tracer::Parse::Parse(vector<pair<TokenType, TokenRange>> const & ranges, bool category)
  : m_category(category)
{
  for (auto const & kv : ranges)
    m_ranges[kv.first] = kv.second;
}

// Tracer ------------------------------------------------------------------------------------------
vector<Tracer::Parse> Tracer::GetUniqueParses() const
{
  auto parses = m_parses;
  base::SortUnique(parses);
  return parses;
}

void Tracer::CallMethod(Branch branch)
{
  m_provenance.emplace_back(branch);
}

void Tracer::LeaveMethod(Branch branch)
{
  CHECK(!m_provenance.empty(), ());
  CHECK_EQUAL(m_provenance.back(), branch, ());
  m_provenance.pop_back();
}

// Functions ---------------------------------------------------------------------------------------
string DebugPrint(Tracer::Parse const & parse)
{
  using TokenType = Tracer::Parse::TokenType;

  ostringstream os;
  os << "Parse [";

  bool first = true;
  for (size_t i = 0; i < TokenType::TOKEN_TYPE_COUNT; ++i)
  {
    auto const & range = parse.m_ranges[i];
    if (range.Begin() == range.End())
      continue;

    if (!first)
      os << ", ";

    os << DebugPrint(static_cast<TokenType>(i)) << ": " << DebugPrint(range);
    first = false;
  }

  os << ", category: " << boolalpha << parse.m_category;
  os << "]";

  return os.str();
}

string DebugPrint(Tracer::Branch branch)
{
  switch (branch)
  {
  case Tracer::Branch::GoEverywhere: return "GoEverywhere";
  case Tracer::Branch::GoInViewport: return "GoInViewport";
  case Tracer::Branch::MatchCategories: return "MatchCategories";
  case Tracer::Branch::MatchRegions: return "MatchRegions";
  case Tracer::Branch::MatchCities: return "MatchCities";
  case Tracer::Branch::MatchAroundPivot: return "MatchAroundPivot";
  case Tracer::Branch::MatchPOIsAndBuildings: return "MatchPOIsAndBuildings";
  case Tracer::Branch::GreedilyMatchStreets: return "GreedilyMatchStreets";
  case Tracer::Branch::WithPostcodes: return "WithPostcodes";
  case Tracer::Branch::MatchUnclassified: return "MatchUnclassified";
  }
  UNREACHABLE();
}
}  // namespace search
