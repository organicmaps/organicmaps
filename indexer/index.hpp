#pragma once
#include "cell_id.hpp"
#include "covering.hpp"
#include "data_header.hpp"
#include "data_header_reader.hpp"
#include "features_vector.hpp"
#include "scale_index.hpp"
#include "scales.hpp"

#include "../../defines.hpp"

#include "../geometry/rect2d.hpp"

#include "../coding/file_container.hpp"
//#include "../coding/varint.hpp"

#include "../base/base.hpp"
#include "../base/macros.hpp"
#include "../base/stl_add.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/unordered_set.hpp"
#include "../std/utility.hpp"
#include "../std/bind.hpp"


template <class BaseT> class IndexForEachAdapter : public BaseT
{
public:
  typedef typename BaseT::Query Query;

  template <typename F>
  void ForEachInRect(F const & f, m2::RectD const & rect, uint32_t scale, Query & query) const
  {
    vector<pair<int64_t, int64_t> > intervals = covering::CoverViewportAndAppendLowerLevels(rect);
    for (size_t i = 0; i < intervals.size(); ++i)
      BaseT::ForEachInIntervalAndScale(f, intervals[i].first, intervals[i].second, scale,
                                       rect, query);
  }

  template <typename F>
  void ForEachInRect(F const & f, m2::RectD const & rect, uint32_t scale) const
  {
    Query query;
    ForEachInRect(f, rect, scale, query);
  }

  template <typename F>
  void ForEachInViewport(F const & f, m2::RectD const & viewport, Query & query) const
  {
    ForEachInRect(f, viewport, scales::GetScaleLevel(viewport), query);
  }

  template <typename F>
  void ForEachInViewport(F const & f, m2::RectD const & viewport) const
  {
    Query query;
    ForEachInViewport(f, viewport, query);
  }

  template <typename F>
  void ForEachInScale(F const & f, uint32_t scale, Query & query) const
  {
    int64_t const rootId = RectId("").ToInt64();
    BaseT::ForEachInIntervalAndScale(f, rootId, rootId + RectId("").SubTreeSize(), scale,
                                     m2::RectD::GetInfiniteRect(), query);
  }

  template <typename F>
  void ForEachInScale(F const & f, uint32_t scale) const
  {
    Query query;
    ForEachInScale(f, scale, query);
  }
};

template <class IndexT> class MultiIndexAdapter
{
public:
  typedef typename IndexT::Query Query;

  ~MultiIndexAdapter()
  {
    Clean();
  }

  template <typename F>
  void ForEachInIntervalAndScale(F const & f, int64_t beg, int64_t end, uint32_t scale,
                                 m2::RectD const & occlusionRect, Query & query) const
  {
    for (size_t i = 0; i < m_Indexes.size(); ++i)
      if (IndexT * pIndex = m_Indexes[i]->GetIndex(scale, occlusionRect))
        pIndex->ForEachInIntervalAndScale(f, beg, end, scale, query);
  }

  void Add(string const & path)
  {
    m_Indexes.push_back(new IndexProxy(path));
  }

  bool IsExist(string const & dataPath) const
  {
    return (find_if(m_Indexes.begin(), m_Indexes.end(),
                    bind(&IndexProxy::IsMyData, _1, cref(dataPath))) !=
            m_Indexes.end());
  }

  void Remove(string const & dataPath)
  {
    for (typename vector<IndexProxy *>::iterator it = m_Indexes.begin();
         it != m_Indexes.end(); ++it)
    {
      if ((*it)->IsMyData(dataPath))
      {
        delete *it;
        m_Indexes.erase(it);
        break;
      }
    }
  }

  void Clean()
  {
    for_each(m_Indexes.begin(), m_Indexes.end(), DeleteFunctor());
    m_Indexes.clear();
  }

private:

  class IndexProxy
  {
  public:
    IndexProxy(string const & path) : m_Path(path), m_pIndex(NULL)
    {
      // TODO: If path is cellid-style-square, make rect from cellid and don't open the file.
      feature::DataHeader header;
      feature::ReadDataHeader(path, header);
      m_Rect = header.Bounds();
    }

    // TODO: GetIndex(), Open() and Close() make Index single-threaded!
    IndexT * GetIndex(uint32_t /*scale*/, m2::RectD const & occlusionRect)
    {
      // TODO: Scale should also be taken into account, to skip irrelevant mwm files.
      if (m_Rect.IsIntersect(occlusionRect))
      {
        Open();
        m_QueriesSkipped = 0;
        return m_pIndex;
      }
      else
      {
        if (++m_QueriesSkipped > 8)
          Close();
        return NULL;
      }
    }

    bool IsMyData(string path) const
    {
      return m_Path == path;
    }

    ~IndexProxy()
    {
      Close();
    }

  private:

    void Open()
    {
      if (!m_pIndex)
      {
        // LOG(LINFO, (m_Path));
        uint32_t const logPageSize = 10;
        uint32_t const logPageCount = 12;
        FilesContainerR container(m_Path, logPageSize, logPageCount);
        m_pIndex = new IndexT(container);
      }
    }

    void Close()
    {
      if (m_pIndex)
      {
        // LOG(LINFO, (m_Path));
        delete m_pIndex;
        m_pIndex = NULL;
      }
    }

    m2::RectD m_Rect;
    string m_Path; // TODO: Store prefix and suffix of path in MultiIndexAdapter.
    IndexT * m_pIndex;
    uint8_t m_QueriesSkipped;
  };

  vector<IndexProxy *> m_Indexes;
};

template <class FeatureVectorT, class BaseT> class OffsetToFeatureAdapter : public BaseT
{
public:
  typedef typename BaseT::Query Query;

  explicit OffsetToFeatureAdapter(FilesContainerR const & container)
  : BaseT(container.GetReader(INDEX_FILE_TAG)),
    m_FeatureVector(container)
  {
  }

  template <typename F>
  void ForEachInIntervalAndScale(F const & f, int64_t beg, int64_t end, uint32_t scale,
                                 Query & query) const
  {
    OffsetToFeatureReplacer<F> offsetToFeatureReplacer(m_FeatureVector, f);
    BaseT::ForEachInIntervalAndScale(offsetToFeatureReplacer, beg, end, scale, query);
  }

  bool IsMyData(string const & fName) const
  {
    return m_FeatureVector.IsMyData(fName);
  }

private:
  FeatureVectorT m_FeatureVector;

  template <typename F>
  class OffsetToFeatureReplacer
  {
    FeatureVectorT const & m_V;
    F const & m_F;

  public:
    OffsetToFeatureReplacer(FeatureVectorT const & v, F const & f) : m_V(v), m_F(f) {}
    void operator() (uint32_t offset) const
    {
      FeatureType feature;
      m_V.Get(offset, feature);
      m_F(feature);
    }
  };
};

template <class BaseT> class UniqueOffsetAdapter : public BaseT
{
public:
  // Defines base Query type.
  // If someone in BaseT want's to do that, use the following line and pass query in ForEachXXX().
  // class Query : public typename BaseT::Query
  class Query
  {
    // TODO: Remember max offsets.size() and initialize offsets with it?
    unordered_set<uint32_t> m_Offsets;
    friend class UniqueOffsetAdapter;
  };

  template <typename T1>
  explicit UniqueOffsetAdapter(T1 const & t1) : BaseT(t1) {}

  template <typename T1, typename T2>
  UniqueOffsetAdapter(T1 const & t1, T2 const & t2) : BaseT(t1, t2) {}

  template <typename F>
  void ForEachInIntervalAndScale(F const & f, int64_t beg, int64_t end, uint32_t scale,
                                 Query & query) const
  {
    UniqueOffsetFunctorAdapter<F> uniqueOffsetFunctorAdapter(query.m_Offsets, f);
    BaseT::ForEachInIntervalAndScale(uniqueOffsetFunctorAdapter, beg, end, scale);
  }

private:
  template <typename F>
  struct UniqueOffsetFunctorAdapter
  {
    UniqueOffsetFunctorAdapter(unordered_set<uint32_t> & offsets, F const & f)
      : m_Offsets(offsets), m_F(f) {}

    void operator() (uint32_t offset) const
    {
      if (m_Offsets.insert(offset).second)
        m_F(offset);
    }

    unordered_set<uint32_t> & m_Offsets;
    F const & m_F;
  };
};

template <typename ReaderT>
struct Index
{
  typedef IndexForEachAdapter<
            MultiIndexAdapter<
              OffsetToFeatureAdapter<FeaturesVector,
                UniqueOffsetAdapter<
                  ScaleIndex<ReaderT>
                >
              >
            >
          > Type;
};
