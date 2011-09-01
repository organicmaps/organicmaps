#include "dumper.hpp"

#include "../coding/multilang_utf8_string.hpp"

#include "../indexer/classificator.hpp"
#include "../indexer/feature_processor.hpp"

#include "../std/algorithm.hpp"
#include "../std/bind.hpp"
#include "../std/iostream.hpp"
#include "../std/map.hpp"
#include "../std/vector.hpp"

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
      if (!f.GetPreferredDrawableName().empty())
        ++m_namesCount;

      m_currFeatureTypes.clear();
      f.ForEachTypeRef(*this);
      CHECK(!m_currFeatureTypes.empty(), ("Feature without any type???"));
      pair<value_type::iterator, bool> found = m_stats.insert(make_pair(m_currFeatureTypes, 1));
      if (!found.second)
        found.first->second++;
    }

    void operator()(uint32_t type)
    {
      m_currFeatureTypes.push_back(type);
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

  typedef map<int8_t, map<string, pair<unsigned int, string> > > TokensContainerT;
  class PrefixesCollector
  {
  public:
    TokensContainerT m_stats;

    bool operator()(int8_t langCode, string const & name)
    {
      CHECK(!name.empty(), ("Feature name is empty"));

      vector<string> tokens;
      strings::SimpleTokenizer tok(name, " ");
      while (tok)
      {
        tokens.push_back(*tok);
        ++tok;
      }

      if (tokens.empty())
        return true;
      /*
      // ignore token if it's first letter is an uppercase letter
      strings::UniString const s1 = strings::MakeUniString(tokens[0]);
      strings::UniString const s2 = strings::MakeLowerCase(s1);
      if (s1[0] != s2[0])
        return true;
      */

      for (size_t i = 1; i < tokens.size(); ++i)
      {
        string s;
        for (size_t numTokens = 0; numTokens < i; ++numTokens)
        {
          s += tokens[numTokens];
          s += " ";
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
    typedef pair<string, pair<unsigned int, string> > NameElemT;
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
        cout << it->second.first << " " << it->first << " \"" << it->second.second << "\"" << endl;
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

}
