#include "generator/dumper.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/classificator.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/search_trie.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/iostream.hpp"
#include "std/map.hpp"
#include "std/queue.hpp"
#include "std/vector.hpp"

namespace feature
{
  class TypesCollector
  {
    vector<uint32_t> m_currFeatureTypes;

  public:
    typedef map<vector<uint32_t>, size_t> value_type;
    value_type m_stats;
    size_t m_namesCount;
    size_t m_totalCount;

    TypesCollector() : m_namesCount(0), m_totalCount(0) {}

    void operator()(FeatureType & f, uint32_t)
    {
      ++m_totalCount;
      string s1, s2;
      f.GetPreferredNames(s1, s2);
      if (!s1.empty())
        ++m_namesCount;

      m_currFeatureTypes.clear();
      f.ForEachType([this](uint32_t type)
      {
        m_currFeatureTypes.push_back(type);
      });
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
    feature::ForEachFromDat(fPath, doClass);

    typedef pair<vector<uint32_t>, size_t> stats_elem_type;
    typedef vector<stats_elem_type> vec_to_sort;
    vec_to_sort vecToSort(doClass.m_stats.begin(), doClass.m_stats.end());
    sort(vecToSort.begin(), vecToSort.end(), &SortFunc<stats_elem_type>);

    for (vec_to_sort::iterator it = vecToSort.begin(); it != vecToSort.end(); ++it)
    {
      cout << it->second << " ";
      for (size_t i = 0; i < it->first.size(); ++i)
        cout << classif().GetFullObjectName(it->first[i]) << " ";
      cout << endl;
    }
    cout << "Total features: " << doClass.m_totalCount << endl;
    cout << "Features with names: " << doClass.m_namesCount << endl;
  }

  ///////////////////////////////////////////////////////////////////

  typedef map<int8_t, map<strings::UniString, pair<unsigned int, string> > > TokensContainerT;
  class PrefixesCollector
  {
  public:
    TokensContainerT m_stats;

    bool operator()(int8_t langCode, string const & name)
    {
      CHECK(!name.empty(), ("Feature name is empty"));

      vector<strings::UniString> tokens;
      search::SplitUniString(search::NormalizeAndSimplifyString(name),
                             MakeBackInsertFunctor(tokens), search::Delimiters());

      if (tokens.empty())
        return true;

      for (size_t i = 1; i < tokens.size(); ++i)
      {
        strings::UniString s;
        for (size_t numTokens = 0; numTokens < i; ++numTokens)
        {
          s.append(tokens[numTokens].begin(), tokens[numTokens].end());
          s.push_back(' ');
        }
        pair<TokensContainerT::mapped_type::iterator, bool> found =
            m_stats[langCode].insert(make_pair(s, make_pair(1U, name)));
        if (!found.second)
          found.first->second.first++;
      }
      return true;
    }

    void operator()(FeatureType & f, uint32_t)
    {
      f.ForEachNameRef(*this);
    }
  };

  static size_t const MIN_OCCURRENCE = 3;

  void Print(int8_t langCode, TokensContainerT::mapped_type const & container)
  {
    typedef pair<strings::UniString, pair<unsigned int, string> > NameElemT;
    typedef vector<NameElemT> VecToSortT;

    VecToSortT v(container.begin(), container.end());
    sort(v.begin(), v.end(), &SortFunc<NameElemT>);

    // do not display prefixes with low occurrences
    if (v[0].second.first > MIN_OCCURRENCE)
    {
      cout << "Language code: " << StringUtf8Multilang::GetLangByCode(langCode) << endl;

      for (VecToSortT::iterator it = v.begin(); it != v.end(); ++it)
      {
        if (it->second.first <= MIN_OCCURRENCE)
          break;
        cout << it->second.first << " " << strings::ToUtf8(it->first);
        cout << " \"" << it->second.second << "\"" << endl;
      }
    }
  }

  void DumpPrefixes(string const & fPath)
  {
    PrefixesCollector doClass;
    feature::ForEachFromDat(fPath, doClass);
    for (TokensContainerT::iterator it = doClass.m_stats.begin();
         it != doClass.m_stats.end(); ++it)
    {
      Print(it->first, it->second);
    }
  }

  struct SearchTokensCollector
  {
    priority_queue<pair<uint32_t, strings::UniString> > tokens;
    strings::UniString m_currentS;
    uint32_t m_currentCount;

    SearchTokensCollector() : m_currentS(), m_currentCount(0) {}

    void operator()(strings::UniString const & s, trie::ValueReader::ValueType const &)
    {
      if (m_currentS == s)
      {
        ++m_currentCount;
      }
      else
      {
        if (m_currentCount > 0)
        {
          tokens.push(make_pair(m_currentCount, m_currentS));
          if (tokens.size() > 100)
            tokens.pop();
        }
        m_currentS = s;
        m_currentCount = 0;
      }
    }

    void Finish()
    {
      if (m_currentCount > 0)
      {
        tokens.push(make_pair(m_currentCount, m_currentS));
        if (tokens.size() > 100)
          tokens.pop();
      }
    }
  };

  void DumpSearchTokens(string const & fPath)
  {
    FilesContainerR container(new FileReader(fPath));
    feature::DataHeader header(container);
    serial::CodingParams cp(trie::GetCodingParams(header.GetDefCodingParams()));

    unique_ptr<trie::DefaultIterator> const pTrieRoot(
        trie::ReadTrie(container.GetReader(SEARCH_INDEX_FILE_TAG), trie::ValueReader(cp),
                       trie::TEdgeValueReader()));

    SearchTokensCollector f;
    trie::ForEachRef(*pTrieRoot, f, strings::UniString());
    f.Finish();

    while (!f.tokens.empty())
    {
      strings::UniString const & s = f.tokens.top().second;
      cout << f.tokens.top().first << " '" << strings::ToUtf8(s) << "'" << endl;
      f.tokens.pop();
    }
  }

}  // namespace feature
