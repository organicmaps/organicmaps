#include "search/search_query_params.hpp"

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
  DoAddStreetSynonyms(SearchQueryParams & params) : m_params(params) {}

  void operator()(SearchQueryParams::TString const & s, size_t i)
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
  SearchQueryParams::TSynonymsVector & GetSyms(size_t i) const
  {
    size_t const count = m_params.m_tokens.size();
    if (i < count)
      return m_params.m_tokens[i];
    ASSERT_EQUAL(i, count, ());
    return m_params.m_prefixTokens;
  }

  void AddSym(size_t i, string const & sym) { GetSyms(i).push_back(strings::MakeUniString(sym)); }

  SearchQueryParams & m_params;
};
}  // namespace

SearchQueryParams::SearchQueryParams() : m_scale(scales::GetUpperScale()) {}

void SearchQueryParams::Clear()
{
  m_tokens.clear();
  m_prefixTokens.clear();
  m_langs.clear();
  m_scale = scales::GetUpperScale();
}

void SearchQueryParams::EraseTokens(vector<size_t> & eraseInds)
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

void SearchQueryParams::ProcessAddressTokens()
{
  // Erases all number tokens.
  // Assumes that USA street name numbers are end with "st, nd, rd, th" suffixes.
  vector<size_t> toErase;
  ForEachToken([&toErase](SearchQueryParams::TString const & s, size_t i)
               {
                 if (feature::IsNumber(s))
                   toErase.push_back(i);
               });
  EraseTokens(toErase);

  // Adds synonyms for N, NE, NW, etc.
  ForEachToken(DoAddStreetSynonyms(*this));
}

template <class ToDo>
void SearchQueryParams::ForEachToken(ToDo && toDo)
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
}  // namespace search

string DebugPrint(search::SearchQueryParams const & params)
{
  ostringstream os;
  os << "SearchQueryParams [ m_tokens=" << DebugPrint(params.m_tokens)
     << ", m_prefixTokens=" << DebugPrint(params.m_prefixTokens) << "]";
  return os.str();
}
