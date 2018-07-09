#include "search/query_params.hpp"

#include "search/ranking_utils.hpp"
#include "search/token_range.hpp"

#include "indexer/feature_impl.hpp"

#include <sstream>

using namespace std;

namespace search
{
namespace
{
// TODO (@y, @m): reuse this class in Processor.
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
      AddSynonym(i, "north");
    if (ss == "w")
      AddSynonym(i, "west");
    if (ss == "s")
      AddSynonym(i, "south");
    if (ss == "e")
      AddSynonym(i, "east");
    if (ss == "nw")
      AddSynonym(i, "northwest");
    if (ss == "ne")
      AddSynonym(i, "northeast");
    if (ss == "sw")
      AddSynonym(i, "southwest");
    if (ss == "se")
      AddSynonym(i, "southeast");
  }

private:
  void AddSynonym(size_t i, string const & synonym) { m_params.GetToken(i).AddSynonym(synonym); }

  QueryParams & m_params;
};
}  // namespace

// QueryParams::Token ------------------------------------------------------------------------------
void QueryParams::Token::AddSynonym(string const & s)
{
  AddSynonym(strings::MakeUniString(s));
}

void QueryParams::Token::AddSynonym(String const & s)
{
  if (!IsStopWord(s))
    m_synonyms.push_back(s);
}

string DebugPrint(QueryParams::Token const & token)
{
  ostringstream os;
  os << "Token [ m_original=" << DebugPrint(token.m_original)
     << ", m_synonyms=" << DebugPrint(token.m_synonyms) << " ]";
  return os.str();
}

// QueryParams -------------------------------------------------------------------------------------
void QueryParams::Clear()
{
  m_tokens.clear();
  m_prefixToken.Clear();
  m_hasPrefix = false;
  m_typeIndices.clear();
  m_langs.Clear();
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

QueryParams::Token const & QueryParams::GetToken(size_t i) const
{
  ASSERT_LESS(i, GetNumTokens(), ());
  return i < m_tokens.size() ? m_tokens[i] : m_prefixToken;
}

QueryParams::Token & QueryParams::GetToken(size_t i)
{
  ASSERT_LESS(i, GetNumTokens(), ());
  return i < m_tokens.size() ? m_tokens[i] : m_prefixToken;
}

bool QueryParams::IsNumberTokens(TokenRange const & range) const
{
  ASSERT(range.IsValid(), (range));
  ASSERT_LESS_OR_EQUAL(range.End(), GetNumTokens(), ());

  for (size_t i : range)
  {
    bool number = false;
    GetToken(i).ForEach([&number](String const & s) {
      if (feature::IsNumber(s))
      {
        number = true;
        return false;  // breaks ForEach
      }
      return true;  // continues ForEach
    });

    if (!number)
      return false;
  }

  return true;
}

void QueryParams::RemoveToken(size_t i)
{
  ASSERT_LESS(i, GetNumTokens(), ());
  if (i == m_tokens.size())
  {
    m_prefixToken.Clear();
    m_hasPrefix = false;
  }
  else
  {
    m_tokens.erase(m_tokens.begin() + i);
  }
  m_typeIndices.erase(m_typeIndices.begin() + i);
}

string DebugPrint(QueryParams const & params)
{
  ostringstream os;
  os << "QueryParams [ m_tokens=" << ::DebugPrint(params.m_tokens)
     << ", m_prefixToken=" << DebugPrint(params.m_prefixToken)
     << ", m_typeIndices=" << ::DebugPrint(params.m_typeIndices)
     << ", m_langs=" << DebugPrint(params.m_langs) << " ]";
  return os.str();
}
}  // namespace search
