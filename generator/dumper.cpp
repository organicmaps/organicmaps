#include "generator/dumper.hpp"

#include "search/search_index_values.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/trie_reader.hpp"

#include "coding/string_utf8_multilang.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <vector>

#include "defines.hpp"

namespace features_dumper
{
using std::cout, std::endl, std::pair, std::string, std::vector;

template <typename Value>
struct SearchTokensCollector
{
  SearchTokensCollector() : m_currentS(), m_currentCount(0) {}

  void operator()(strings::UniString const & s, Value const & /* value */)
  {
    if (m_currentS != s)
    {
      if (m_currentCount > 0)
        m_tokens.emplace_back(m_currentCount, m_currentS);
      m_currentS = s;
      m_currentCount = 0;
    }
    ++m_currentCount;
  }

  void Finish()
  {
    if (m_currentCount > 0)
      m_tokens.emplace_back(m_currentCount, m_currentS);
    sort(m_tokens.begin(), m_tokens.end(), std::greater<pair<uint32_t, strings::UniString>>());
  }

  vector<pair<uint32_t, strings::UniString>> m_tokens;
  strings::UniString m_currentS;
  uint32_t m_currentCount;
};

class TypesCollector
{
  vector<uint32_t> m_currFeatureTypes;

public:
  typedef std::map<vector<uint32_t>, size_t> value_type;
  value_type m_stats;
  size_t m_namesCount;
  size_t m_totalCount;

  TypesCollector() : m_namesCount(0), m_totalCount(0) {}

  void operator()(FeatureType & f, uint32_t)
  {
    ++m_totalCount;
    auto const primary = f.GetReadableName();
    if (!primary.empty())
      ++m_namesCount;

    m_currFeatureTypes.clear();
    f.ForEachType([this](uint32_t type) { m_currFeatureTypes.push_back(type); });
    CHECK(!m_currFeatureTypes.empty(), ("Feature without any type???"));

    auto found = m_stats.insert(make_pair(m_currFeatureTypes, 1));
    if (!found.second)
      found.first->second++;
  }
};

template <class T>
static bool SortFunc(T const & first, T const & second)
{
  return first.second > second.second;
}

void DumpTypes(string const & fPath)
{
  TypesCollector doClass;
  feature::ForEachFeature(fPath, doClass);

  typedef pair<vector<uint32_t>, size_t> stats_elem_type;
  typedef vector<stats_elem_type> vec_to_sort;
  vec_to_sort vecToSort(doClass.m_stats.begin(), doClass.m_stats.end());
  sort(vecToSort.begin(), vecToSort.end(), &SortFunc<stats_elem_type>);

  for (auto const & el : vecToSort)
  {
    cout << el.second << " ";
    for (uint32_t i : el.first)
      cout << classif().GetFullObjectName(i) << " ";
    cout << endl;
  }
  cout << "Total features: " << doClass.m_totalCount << endl;
  cout << "Features with names: " << doClass.m_namesCount << endl;
}

///////////////////////////////////////////////////////////////////

typedef std::map<int8_t, std::map<strings::UniString, pair<unsigned int, string>>> TokensContainerT;
class PrefixesCollector
{
public:
  TokensContainerT m_stats;

  void operator()(int8_t langCode, std::string_view name)
  {
    CHECK(!name.empty(), ("Feature name is empty"));

    auto const tokens = search::NormalizeAndTokenizeString(name);
    for (size_t i = 1; i < tokens.size(); ++i)
    {
      strings::UniString s;
      for (size_t numTokens = 0; numTokens < i; ++numTokens)
      {
        s.append(tokens[numTokens].begin(), tokens[numTokens].end());
        s.push_back(' ');
      }
      auto [iter, found] = m_stats[langCode].emplace(s, std::make_pair(1U, name));
      if (!found)
        iter->second.first++;
    }
  }

  void operator()(FeatureType & f, uint32_t) { f.ForEachName(*this); }
};

static size_t constexpr MIN_OCCURRENCE = 3;

void Print(int8_t langCode, TokensContainerT::mapped_type const & container)
{
  typedef pair<strings::UniString, pair<unsigned int, string>> NameElemT;
  typedef vector<NameElemT> VecToSortT;

  VecToSortT v(container.begin(), container.end());
  std::sort(v.begin(), v.end(), &SortFunc<NameElemT>);

  // do not display prefixes with low occurrences
  if (v[0].second.first > MIN_OCCURRENCE)
  {
    cout << "Language code: " << StringUtf8Multilang::GetLangByCode(langCode) << endl;

    for (auto const & el : v)
    {
      if (el.second.first <= MIN_OCCURRENCE)
        break;
      cout << el.second.first << " " << strings::ToUtf8(el.first);
      cout << " \"" << el.second.second << "\"" << endl;
    }
  }
}

void DumpPrefixes(string const & fPath)
{
  PrefixesCollector doClass;
  feature::ForEachFeature(fPath, doClass);
  for (auto const & [langCode, container] : doClass.m_stats)
    Print(langCode, container);
}

void DumpSearchTokens(string const & fPath, size_t maxTokensToShow)
{
  using Value = Uint64IndexValue;

  FilesContainerR container(std::make_unique<FileReader>(fPath));
  feature::DataHeader header(container);

  auto const trieRoot = trie::ReadTrie<ModelReaderPtr, ValueList<Value>>(container.GetReader(SEARCH_INDEX_FILE_TAG),
                                                                         SingleValueSerializer<Value>());

  SearchTokensCollector<Value> f;
  trie::ForEachRef(*trieRoot, f, strings::UniString());
  f.Finish();

  for (size_t i = 0; i < std::min(maxTokensToShow, f.m_tokens.size()); ++i)
  {
    auto const & s = f.m_tokens[i].second;
    cout << f.m_tokens[i].first << " " << strings::ToUtf8(s) << endl;
  }
}

void DumpFeatureNames(string const & fPath, string const & lang)
{
  int8_t const langIndex = StringUtf8Multilang::GetLangIndex(lang);

  feature::ForEachFeature(fPath, [&](FeatureType & f, uint32_t)
  {
    f.ForEachName([&](int8_t langCode, std::string_view name)
    {
      CHECK(!name.empty(), ("Feature name is empty"));

      if (langIndex == StringUtf8Multilang::kUnsupportedLanguageCode)
        cout << StringUtf8Multilang::GetLangByCode(langCode) << ' ' << name << endl;
      else if (langCode == langIndex)
        cout << name << endl;
    });
  });
}
}  // namespace features_dumper
