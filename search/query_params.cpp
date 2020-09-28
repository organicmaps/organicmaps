#include "search/query_params.hpp"

#include "search/ranking_utils.hpp"
#include "search/token_range.hpp"

#include "indexer/feature_impl.hpp"

#include <map>
#include <sstream>

using namespace std;
using namespace strings;

namespace search
{
namespace
{
// All synonyms should be lowercase.
map<string, vector<string>> const kSynonyms = {
    {"n",    {"north"}},
    {"w",    {"west"}},
    {"s",    {"south"}},
    {"e",    {"east"}},
    {"nw",   {"northwest"}},
    {"ne",   {"northeast"}},
    {"sw",   {"southwest"}},
    {"se",   {"southeast"}},
    {"st",   {"saint", "street"}},
    {"св",   {"святой", "святого", "святая", "святые", "святых", "свято"}},
    {"б",    {"большая", "большой"}},
    {"бол",  {"большая", "большой"}},
    {"м",    {"малая", "малый"}},
    {"мал",  {"малая", "малый"}},
    {"нов",  {"новая", "новый"}},
    {"стар", {"старая", "старый"}}};
}  // namespace

// QueryParams::Token ------------------------------------------------------------------------------
void QueryParams::Token::AddSynonym(string const & s) { AddSynonym(MakeUniString(s)); }

void QueryParams::Token::AddSynonym(String const & s)
{
  if (!IsStopWord(s))
    m_synonyms.push_back(s);
}

string DebugPrint(QueryParams::Token const & token)
{
  ostringstream os;
  os << "Token [ m_original=" << DebugPrint(token.GetOriginal())
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
    if (!GetToken(i).AnyOfOriginalOrSynonyms([](String const & s) { return feature::IsNumber(s); }))
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

void QueryParams::AddSynonyms()
{
  for (auto & token : m_tokens)
  {
    string const ss = ToUtf8(MakeLowerCase(token.GetOriginal()));
    auto const it = kSynonyms.find(ss);
    if (it == kSynonyms.end())
      continue;

    for (auto const & synonym : it->second)
      token.AddSynonym(synonym);
  }
}

string DebugPrint(QueryParams const & params)
{
  ostringstream os;
  os << "QueryParams [ "
     << "m_query=\"" << params.m_query << "\""
     << ", m_tokens=" << ::DebugPrint(params.m_tokens)
     << ", m_prefixToken=" << DebugPrint(params.m_prefixToken)
     << ", m_typeIndices=" << ::DebugPrint(params.m_typeIndices)
     << ", m_langs=" << DebugPrint(params.m_langs) << " ]";
  return os.str();
}
}  // namespace search
