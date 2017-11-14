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

private:
  std::vector<Parse> m_parses;
};

std::string DebugPrint(Tracer::Parse const & parse);
}  // namespace search
