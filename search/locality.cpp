#include "search/locality.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/limits.hpp"

namespace search
{
Locality::Locality()
  : m_type(ftypes::NONE)
  , m_featureId(numeric_limits<decltype(m_featureId)>::max())
  , m_rank(numeric_limits<decltype(m_rank)>::max())
  , m_radius(0)
{
}

Locality::Locality(ftypes::Type type, uint32_t featureId, m2::PointD const & center, uint8_t rank)
  : m_type(type), m_featureId(featureId), m_center(center), m_rank(rank), m_radius(0)
{
}

bool Locality::IsValid() const
{
  if (m_type == ftypes::NONE)
    return false;
  ASSERT(!m_matchedTokens.empty(), ());
  return true;
}

bool Locality::IsSuitable(TTokensArray const & tokens, TToken const & prefix) const
{
  bool const isMatched = IsFullNameMatched();

  // Do filtering of possible localities.
  using namespace ftypes;

  switch (m_type)
  {
    case COUNTRY:
      // USA has synonyms: "US" or "USA"
      return (isMatched || (m_enName == "usa" && GetSynonymTokenLength(tokens, prefix) <= 3) ||
              (m_enName == "uk" && GetSynonymTokenLength(tokens, prefix) == 2));

    case STATE:  // we process USA, Canada states only for now
      // USA states has 2-symbol synonyms
      return (isMatched || GetSynonymTokenLength(tokens, prefix) == 2);
    case CITY:
      // need full name match for cities
      return isMatched;
    case NONE:
    case TOWN:
    case VILLAGE:
    case LOCALITY_COUNT:
      ASSERT(false, ("Unsupported type:", m_type));
      return false;
  }
}

void Locality::Swap(Locality & rhs)
{
  m_name.swap(rhs.m_name);
  m_enName.swap(rhs.m_enName);
  m_matchedTokens.swap(rhs.m_matchedTokens);

  swap(m_type, rhs.m_type);
  swap(m_featureId, rhs.m_featureId);
  swap(m_center, rhs.m_center);
  swap(m_rank, rhs.m_rank);
  swap(m_radius, rhs.m_radius);
}

bool Locality::operator<(Locality const & rhs) const
{
  if (m_type != rhs.m_type)
    return (m_type < rhs.m_type);

  if (m_matchedTokens.size() != rhs.m_matchedTokens.size())
    return (m_matchedTokens.size() < rhs.m_matchedTokens.size());

  return m_rank < rhs.m_rank;
}

bool Locality::IsFullNameMatched() const
{
  size_t count = 0;
  SplitUniString(NormalizeAndSimplifyString(m_name), [&count](strings::UniString const &)
                 {
                   ++count;
                 },
                 search::Delimiters());
  return count <= m_matchedTokens.size();
}

size_t Locality::GetSynonymTokenLength(TTokensArray const & tokens, TToken const & prefix) const
{
  // check only one token as a synonym
  if (m_matchedTokens.size() == 1)
  {
    size_t const index = m_matchedTokens[0];
    if (index < tokens.size())
      return tokens[index].size();
    ASSERT_EQUAL(index, tokens.size(), ());
    ASSERT(!prefix.empty(), ());
    return prefix.size();
  }

  return size_t(-1);
}

string DebugPrint(Locality const & l)
{
  stringstream ss;
  ss << "{ Locality: "
     << "Name = " + l.m_name << "; Name English = " << l.m_enName
     << "; Rank = " << static_cast<int>(l.m_rank)
     << "; Matched: " << l.m_matchedTokens.size() << " }";
  return ss.str();
}
}  // namespace search
