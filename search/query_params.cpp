#include "search/query_params.hpp"

#include "indexer/feature_impl.hpp"

#include "std/algorithm.hpp"

namespace search
{
namespace
{
// TODO (@y, @m): reuse this class in search::Processor.
class DoAddStreetSynonyms
{
public:
  DoAddStreetSynonyms(QueryParams & params) : m_params(params) {}

  void operator()(QueryParams::String const & s, size_t i)
  {
    if (s.size() > 2)
      return;
    string const ss = strings::ToUtf8(strings::MakeLowerCase(s));

    // All synonyms should be lowercase!
    if (ss == "n")
      AddSym(i, "north");
    if (ss == "w")
      AddSym(i, "west");
    if (ss == "s")
      AddSym(i, "south");
    if (ss == "e")
      AddSym(i, "east");
    if (ss == "nw")
      AddSym(i, "northwest");
    if (ss == "ne")
      AddSym(i, "northeast");
    if (ss == "sw")
      AddSym(i, "southwest");
    if (ss == "se")
      AddSym(i, "southeast");
  }

private:
  QueryParams::Synonyms & GetSyms(size_t i) const { return m_params.GetTokens(i); }
  void AddSym(size_t i, string const & sym) { GetSyms(i).push_back(strings::MakeUniString(sym)); }

  QueryParams & m_params;
};
}  // namespace

void QueryParams::Clear()
{
  m_tokens.clear();
  m_prefixTokens.clear();
  m_typeIndices.clear();
  m_langs.clear();
  m_scale = scales::GetUpperScale();
}

bool QueryParams::IsCategorySynonym(size_t i) const { return !GetTypeIndices(i).empty(); }

QueryParams::TypeIndices & QueryParams::GetTypeIndices(size_t i)
{
  ASSERT_LESS(i, GetNumTokens(), ());
  return m_typeIndices[i];
}

QueryParams::TypeIndices const & QueryParams::GetTypeIndices(size_t i) const
{
  ASSERT_LESS(i, GetNumTokens(), ());
  return m_typeIndices[i];
}

bool QueryParams::IsPrefixToken(size_t i) const
{
  ASSERT_LESS(i, GetNumTokens(), ());
  return i == m_tokens.size();
}

QueryParams::Synonyms const & QueryParams::GetTokens(size_t i) const
{
  ASSERT_LESS(i, GetNumTokens(), ());
  return i < m_tokens.size() ? m_tokens[i] : m_prefixTokens;
}

QueryParams::Synonyms & QueryParams::GetTokens(size_t i)
{
  ASSERT_LESS(i, GetNumTokens(), ());
  return i < m_tokens.size() ? m_tokens[i] : m_prefixTokens;
}

bool QueryParams::IsNumberTokens(size_t start, size_t end) const
{
  ASSERT_LESS(start, end, ());
  ASSERT_LESS_OR_EQUAL(end, GetNumTokens(), ());

  for (; start != end; ++start)
  {
    bool number = false;
    for (auto const & t : GetTokens(start))
    {
      if (feature::IsNumber(t))
      {
        number = true;
        break;
      }
    }
    if (!number)
      return false;
  }

  return true;
}

void QueryParams::RemoveToken(size_t i)
{
  ASSERT_LESS(i, GetNumTokens(), ());
  if (i == m_tokens.size())
    m_prefixTokens.clear();
  else
    m_tokens.erase(m_tokens.begin() + i);
  m_typeIndices.erase(m_typeIndices.begin() + i);
}

string DebugPrint(search::QueryParams const & params)
{
  ostringstream os;
  os << "QueryParams [ m_tokens=" << DebugPrint(params.m_tokens)
     << ", m_prefixTokens=" << DebugPrint(params.m_prefixTokens)
     << ", m_typeIndices=" << ::DebugPrint(params.m_typeIndices)
     << ", m_langs=" << ::DebugPrint(params.m_langs) << " ]";
  return os.str();
}
}  // namespace search
