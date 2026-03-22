#include "search/tracer.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <cstddef>
#include <iomanip>
#include <sstream>

namespace search
{
// Tracer::Parse -----------------------------------------------------------------------------------
Tracer::Parse::Parse(std::vector<TokenType> const & types, bool category) : m_category(category)
{
  size_t i = 0;
  while (i != types.size())
  {
    auto const type = types[i];
    auto j = i + 1;
    while (j != types.size() && types[j] == type)
      ++j;
    if (type < TokenType::TOKEN_TYPE_COUNT)
      m_ranges[type] = TokenRange(i, j);
    i = j;
  }
}

Tracer::Parse::Parse(std::vector<std::pair<TokenType, TokenRange>> const & ranges, bool category) : m_category(category)
{
  for (auto const & kv : ranges)
    m_ranges[kv.first] = kv.second;
}

// Tracer ------------------------------------------------------------------------------------------
std::vector<Tracer::Parse> Tracer::GetUniqueParses() const
{
  auto parses = m_parses;
  base::SortUnique(parses);
  return parses;
}

// ResultTracer ------------------------------------------------------------------------------------
void ResultTracer::Clear()
{
  m_provenance.clear();
}

void ResultTracer::CallMethod(Branch branch)
{
  m_provenance.emplace_back(branch);
}

void ResultTracer::LeaveMethod(Branch branch)
{
  CHECK(!m_provenance.empty(), ());
  CHECK_EQUAL(m_provenance.back(), branch, ());
  m_provenance.pop_back();
}

// Functions ---------------------------------------------------------------------------------------
std::string DebugPrint(Tracer::Parse const & parse)
{
  using TokenType = Tracer::Parse::TokenType;

  std::ostringstream os;
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

  os << ", category: " << std::boolalpha << parse.m_category;
  os << "]";

  return os.str();
}

std::string DebugPrint(ResultTracer::Branch branch)
{
  switch (branch)
  {
  case ResultTracer::Branch::GoEverywhere: return "GoEverywhere";
  case ResultTracer::Branch::GoInViewport: return "GoInViewport";
  case ResultTracer::Branch::MatchCategories: return "MatchCategories";
  case ResultTracer::Branch::MatchRegions: return "MatchRegions";
  case ResultTracer::Branch::MatchCities: return "MatchCities";
  case ResultTracer::Branch::MatchAroundPivot: return "MatchAroundPivot";
  case ResultTracer::Branch::MatchPOIsAndBuildings: return "MatchPOIsAndBuildings";
  case ResultTracer::Branch::GreedilyMatchStreets: return "GreedilyMatchStreets";
  case ResultTracer::Branch::GreedilyMatchStreetsWithSuburbs: return "GreedilyMatchStreetsWithSuburbs";
  case ResultTracer::Branch::WithPostcodes: return "WithPostcodes";
  case ResultTracer::Branch::MatchUnclassified: return "MatchUnclassified";
  case ResultTracer::Branch::Relaxed: return "Relaxed";
  }
  UNREACHABLE();
}
}  // namespace search
