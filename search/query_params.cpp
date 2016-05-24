#include "search/query_params.hpp"

#include "indexer/feature_impl.hpp"
#include "indexer/scales.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"

namespace search
{
namespace
{
class DoAddStreetSynonyms
{
public:
  DoAddStreetSynonyms(QueryParams & params) : m_params(params) {}

  void operator()(QueryParams::TString const & s, size_t i)
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
  QueryParams::TSynonymsVector & GetSyms(size_t i) const
  {
    size_t const count = m_params.m_tokens.size();
    if (i < count)
      return m_params.m_tokens[i];
    ASSERT_EQUAL(i, count, ());
    return m_params.m_prefixTokens;
  }

  void AddSym(size_t i, string const & sym) { GetSyms(i).push_back(strings::MakeUniString(sym)); }

  QueryParams & m_params;
};
}  // namespace

QueryParams::QueryParams() : m_scale(scales::GetUpperScale()) {}

void QueryParams::Clear()
{
  m_tokens.clear();
  m_prefixTokens.clear();
  m_isCategorySynonym.clear();
  m_langs.clear();
  m_scale = scales::GetUpperScale();
}

void QueryParams::EraseTokens(vector<size_t> & eraseInds)
{
  eraseInds.erase(unique(eraseInds.begin(), eraseInds.end()), eraseInds.end());
  ASSERT(is_sorted(eraseInds.begin(), eraseInds.end()), ());

  // fill temporary vector
  vector<TSynonymsVector> newTokens;

  size_t skipI = 0;
  size_t const count = m_tokens.size();
  size_t const eraseCount = eraseInds.size();
  for (size_t i = 0; i < count; ++i)
  {
    if (skipI < eraseCount && eraseInds[skipI] == i)
      ++skipI;
    else
      newTokens.push_back(move(m_tokens[i]));
  }

  // assign to m_tokens
  newTokens.swap(m_tokens);

  if (skipI < eraseCount)
  {
    // it means that we need to skip prefix tokens
    ASSERT_EQUAL(skipI + 1, eraseCount, (eraseInds));
    ASSERT_EQUAL(eraseInds[skipI], count, (eraseInds));
    m_prefixTokens.clear();
  }
}

void QueryParams::ProcessAddressTokens()
{
  // Erases all number tokens.
  // Assumes that USA street name numbers are end with "st, nd, rd, th" suffixes.
  vector<size_t> toErase;
  ForEachToken([&toErase](QueryParams::TString const & s, size_t i)
               {
                 if (feature::IsNumber(s))
                   toErase.push_back(i);
               });
  EraseTokens(toErase);

  // Adds synonyms for N, NE, NW, etc.
  ForEachToken(DoAddStreetSynonyms(*this));
}

QueryParams::TSynonymsVector const & QueryParams::GetTokens(size_t i) const
{
  ASSERT_LESS_OR_EQUAL(i, m_tokens.size(), ());
  return i < m_tokens.size() ? m_tokens[i] : m_prefixTokens;
}

QueryParams::TSynonymsVector & QueryParams::GetTokens(size_t i)
{
  ASSERT_LESS_OR_EQUAL(i, m_tokens.size(), ());
  return i < m_tokens.size() ? m_tokens[i] : m_prefixTokens;
}

bool QueryParams::IsNumberTokens(size_t start, size_t end) const
{
  ASSERT_LESS(start, end, ());
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

template <class ToDo>
void QueryParams::ForEachToken(ToDo && toDo)
{
  size_t const count = m_tokens.size();
  for (size_t i = 0; i < count; ++i)
  {
    ASSERT(!m_tokens[i].empty(), ());
    ASSERT(!m_tokens[i].front().empty(), ());
    toDo(m_tokens[i].front(), i);
  }

  if (!m_prefixTokens.empty())
  {
    ASSERT(!m_prefixTokens.front().empty(), ());
    toDo(m_prefixTokens.front(), count);
  }
}

string DebugPrint(search::QueryParams const & params)
{
  ostringstream os;
  os << "QueryParams [ m_tokens=" << DebugPrint(params.m_tokens)
     << ", m_prefixTokens=" << DebugPrint(params.m_prefixTokens) << "]";
  return os.str();
}
}  // namespace search
