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

namespace
{
using Id = uint64_t;

class Doc
{
public:
  Doc(std::string const & text, std::string const & lang) : m_lang(StringUtf8Multilang::GetLangIndex(lang))
  {
    m_tokens = search::NormalizeAndTokenizeString(text);
  }

  template <typename ToDo>
  void ForEachToken(ToDo && toDo) const
  {
    for (auto const & token : m_tokens)
      toDo(m_lang, token);
  }

private:
  std::vector<strings::UniString> m_tokens;
  int8_t m_lang;
};

class MemSearchIndexTest
{
public:
  using Index = search_base::MemSearchIndex<Id>;

  void Add(Id const & id, Doc const & doc) { m_index.Add(id, doc); }

  void Erase(Id const & id, Doc const & doc) { m_index.Erase(id, doc); }

  std::vector<Id> StrictQuery(std::string const & query, std::string const & lang) const
  {
    auto prev = m_index.GetAllIds();
    TEST(base::IsSortedAndUnique(prev), ());

    search::ForEachNormalizedToken(query, [&](strings::UniString const & token)
    {
      search::SearchTrieRequest<strings::UniStringDFA> request;
      request.m_names.emplace_back(token);
      request.m_langs.insert(StringUtf8Multilang::GetLangIndex(lang));

      std::vector<Id> curr;
      search::MatchFeaturesInTrie(request, m_index.GetRootIterator(), [](Id const & /* id */)
      { return true; } /* filter */, [&curr](Id const & id, bool /* exactMatch */) { curr.push_back(id); } /* toDo */);
      base::SortUnique(curr);

      std::vector<Id> intersection;
      std::set_intersection(prev.begin(), prev.end(), curr.begin(), curr.end(), std::back_inserter(intersection));
      prev = intersection;
    });

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

  TEST_EQUAL(StrictQuery("Thunder", "en"), std::vector<Id>({kMacbeth}), ());
  TEST_EQUAL(StrictQuery("Question", "en"), std::vector<Id>({kHamlet}), ());
  TEST_EQUAL(StrictQuery("or", "en"), std::vector<Id>({kHamlet, kMacbeth}), ());
  TEST_EQUAL(StrictQuery("thunder lightning rain", "en"), std::vector<Id>({kMacbeth}), ());

  Erase(kMacbeth, macbeth);

  TEST_EQUAL(StrictQuery("Thunder", "en"), std::vector<Id>{}, ());
  TEST_EQUAL(StrictQuery("to be or not to be", "en"), std::vector<Id>({kHamlet}), ());

  Erase(kHamlet, hamlet);
  TEST_EQUAL(StrictQuery("question", "en"), std::vector<Id>{}, ());
}
}  // namespace
