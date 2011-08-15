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
  public:
    typedef map<uint32_t, size_t> value_type;
    value_type m_stats;

    size_t m_namesCount;
    size_t m_totalCount;

    TypesCollector() : m_namesCount(0), m_totalCount(0) {}

    void operator() (FeatureType & f, uint32_t)
    {
      ++m_totalCount;
      if (!f.GetPreferredDrawableName().empty())
        ++m_namesCount;

      f.ForEachTypeRef(*this);
    }

    void operator() (uint32_t type)
    {
      ++m_stats[type];
    }
  };

  struct GreaterSecond
  {
    template <class T> bool operator() (T const & t1, T const & t2)
    {
      return (t1.second > t2.second);
    }
  };

  namespace
  {
    void CallCollect(ClassifObject * p, vector<string> & path, TypesCollector & c);

    void CollectRecursive(ClassifObject * p, vector<string> & path, TypesCollector & c)
    {
      path.push_back(p->GetName());

      c(classif().GetTypeByPath(path));
      CallCollect(p, path, c);

      path.pop_back();
    }

    void CallCollect(ClassifObject * p, vector<string> & path, TypesCollector & c)
    {
      p->ForEachObject(bind(&CollectRecursive, _1, ref(path), ref(c)));
    }
  }

  void DumpTypes(string const & fPath)
  {
    // get all types from mwm file
    TypesCollector doClass;
    feature::ForEachFromDat(fPath, doClass);

    // add types from initial classificator (to get full mapping)
    vector<string> path;
    CallCollect(classif().GetMutableRoot(), path, doClass);

    // sort types by frequency
    typedef vector<pair<uint32_t, size_t> > vec_to_sort;
    vec_to_sort vecToSort(doClass.m_stats.begin(), doClass.m_stats.end());
    sort(vecToSort.begin(), vecToSort.end(), GreaterSecond());

    // print types to out stream
    Classificator & c = classif();
    for (vec_to_sort::iterator i = vecToSort.begin(); i != vecToSort.end(); ++i)
      cout << c.GetFullObjectName(i->first) << endl;

    //cout << "Total features: " << doClass.m_totalCount << endl;
    //cout << "Features with names: " << doClass.m_namesCount << endl;
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
    sort(vecToSort.begin(), vecToSort.end(), GreaterSecond());

    for (VecToSortT::iterator it = vecToSort.begin(); it != vecToSort.end(); ++it)
      cout << it->second << " " << it->first << endl;
  }
}
