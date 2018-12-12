#include "testing/testing.hpp"

#include "search/base/mem_search_index.hpp"
#include "search/feature_offset_match.hpp"

#include "indexer/search_string_utils.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"
#include "base/uni_string_dfa.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

using namespace search_base;
using namespace search;
using namespace std;
using namespace strings;

namespace
{
using Id = uint64_t;

class Doc
{
public:
  Doc(string const & text, string const & lang) : m_lang(StringUtf8Multilang::GetLangIndex(lang))
  {
    NormalizeAndTokenizeString(text, m_tokens);
  }

  template <typename ToDo>
  void ForEachToken(ToDo && toDo) const
  {
    for (auto const & token : m_tokens)
      toDo(m_lang, token);
  }

private:
  vector<strings::UniString> m_tokens;
  int8_t m_lang;
};

class MemSearchIndexTest
{
public:
  using Index = MemSearchIndex<Id>;

  void Add(Id const & id, Doc const & doc) { m_index.Add(id, doc); }

  void Erase(Id const & id, Doc const & doc) { m_index.Erase(id, doc); }

  vector<Id> StrictQuery(string const & query, string const & lang) const
  {
    auto prev = m_index.GetAllIds();
    TEST(base::IsSortedAndUnique(prev.cbegin(), prev.cend()), ());

    vector<UniString> tokens;
    NormalizeAndTokenizeString(query, tokens);
    for (auto const & token : tokens)
    {
      SearchTrieRequest<UniStringDFA> request;
      request.m_names.emplace_back(token);
      request.m_langs.insert(StringUtf8Multilang::GetLangIndex(lang));

      vector<Id> curr;
      MatchFeaturesInTrie(request, m_index.GetRootIterator(),
                          [](Id const & /* id */) { return true; } /* filter */,
                          [&curr](Id const & id) { curr.push_back(id); } /* toDo */);
      base::SortUnique(curr);

      vector<Id> intersection;
      set_intersection(prev.begin(), prev.end(), curr.begin(), curr.end(),
                       back_inserter(intersection));
      prev = intersection;
    }

    return prev;
  }

protected:
  Index m_index;
};

UNIT_CLASS_TEST(MemSearchIndexTest, Smoke)
{
  Id const kHamlet{31337};
  Id const kMacbeth{600613};
  Doc const hamlet{"To be or not to be: that is the question...", "en"};
  Doc const macbeth{"When shall we three meet again? In thunder, lightning, or in rain? ...", "en"};

  Add(kHamlet, hamlet);
  Add(kMacbeth, macbeth);

  TEST_EQUAL(StrictQuery("Thunder", "en"), vector<Id>({kMacbeth}), ());
  TEST_EQUAL(StrictQuery("Question", "en"), vector<Id>({kHamlet}), ());
  TEST_EQUAL(StrictQuery("or", "en"), vector<Id>({kHamlet, kMacbeth}), ());
  TEST_EQUAL(StrictQuery("thunder lightning rain", "en"), vector<Id>({kMacbeth}), ());

  Erase(kMacbeth, macbeth);

  TEST_EQUAL(StrictQuery("Thunder", "en"), vector<Id>{}, ());
  TEST_EQUAL(StrictQuery("to be or not to be", "en"), vector<Id>({kHamlet}), ());

  Erase(kHamlet, hamlet);
  TEST_EQUAL(StrictQuery("question", "en"), vector<Id>{}, ());
}
}  // namespace
