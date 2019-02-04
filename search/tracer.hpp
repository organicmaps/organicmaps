#pragma once

#include "search/geocoder_context.hpp"
#include "search/token_range.hpp"

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace search
{
class Tracer
{
public:
  // Mimics the Geocoder methods.
  enum class Branch
  {
    GoEverywhere,
    GoInViewport,
    MatchCategories,
    MatchRegions,
    MatchCities,
    MatchAroundPivot,
    MatchPOIsAndBuildings,
    GreedilyMatchStreets,
    WithPostcodes,
    MatchUnclassified,
  };

  using Provenance = std::vector<Branch>;

  struct Parse
  {
    using TokenType = BaseContext::TokenType;

    explicit Parse(std::vector<TokenType> const & types, bool category = false);
    explicit Parse(std::vector<std::pair<TokenType, TokenRange>> const & ranges,
                   bool category = false);

    bool operator==(Parse const & rhs) const
    {
      return m_ranges == rhs.m_ranges && m_category == rhs.m_category;
    }

    bool operator<(Parse const & rhs) const
    {
      if (m_ranges != rhs.m_ranges)
        return m_ranges < rhs.m_ranges;
      return m_category < rhs.m_category;
    }

    std::array<TokenRange, TokenType::TOKEN_TYPE_COUNT> m_ranges;
    bool m_category = false;
  };

  template <typename ...Args>
  void EmitParse(Args &&... args)
  {
    m_parses.emplace_back(std::forward<Args>(args)...);
  }

  std::vector<Parse> GetUniqueParses() const;

  void CallMethod(Branch branch);
  void LeaveMethod(Branch branch);
  Provenance const & GetProvenance() const { return m_provenance; }

private:
  std::vector<Parse> m_parses;

  // Traces the Geocoder call tree that leads to emitting the current result.
  Provenance m_provenance;
};

std::string DebugPrint(Tracer::Parse const & parse);
std::string DebugPrint(Tracer::Branch branch);
}  // namespace search
