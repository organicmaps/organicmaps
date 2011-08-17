#include "dumper.hpp"

#include "../indexer/feature_processor.hpp"
#include "../indexer/classificator.hpp"

#include "../std/vector.hpp"
#include "../std/unordered_map.hpp"
#include "../std/iostream.hpp"

namespace feature
{
  class TypesCollector
  {
    vector<uint32_t> m_currFeatureTypes;

  public:
    typedef unordered_map<vector<uint32_t>, size_t> value_type;
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

  class NamesCollector
  {
    typedef unordered_map<string, size_t> NamesContainerT;

    class LangsFunctor
    {
    public:
      vector<string> m_names;
      bool operator()(signed char, string const & name)
      {
        m_names.push_back(name);
        return true;
      }
    };

  public:
    NamesContainerT m_stats;
    void operator()(FeatureType & f, uint32_t)
    {
      LangsFunctor doLangs;
      f.ForEachNameRef(doLangs);
      for (size_t i = 0; i < doLangs.m_names.size(); ++i)
      {
        strings::SimpleTokenizer tok(doLangs.m_names[i], " ");
        while (tok)
        {
          pair<NamesContainerT::iterator, bool> found = m_stats.insert(make_pair(*tok, 1));
          if (!found.second)
            found.first->second++;
          ++tok;
        }
      }
    }
  };

  typedef pair<string, size_t> NameElemT;
  void DumpNames(string const & fPath)
  {
    NamesCollector doClass;
    feature::ForEachFromDat(fPath, doClass);

    typedef vector<NameElemT> VecToSortT;
    VecToSortT vecToSort(doClass.m_stats.begin(), doClass.m_stats.end());
    sort(vecToSort.begin(), vecToSort.end(), &SortFunc<NameElemT>);

    for (VecToSortT::iterator it = vecToSort.begin(); it != vecToSort.end(); ++it)
      cout << it->second << " " << it->first << endl;
  }

}
