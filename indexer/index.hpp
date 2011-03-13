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
#include "../base/mutex.hpp"
#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"
#include "../std/bind.hpp"
#include "../std/string.hpp"
#include "../std/unordered_set.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"

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
    size_t iIndex = 0;
    m_pActiveForEachIndex = NULL;
    m_eActiveForEachIndexAction = ACTIVE_FOR_EACH_INDEX_DO_NOTHING;

    while (true)
    {
      {
        threads::MutexGuard mutexGuard(m_Mutex);
        UNUSED_VALUE(mutexGuard);

        // Do the operation that was delayed because ForEach was active.
        if (m_pActiveForEachIndex)
        {
          switch (m_eActiveForEachIndexAction)
          {
          case ACTIVE_FOR_EACH_INDEX_DO_NOTHING:
            break;
          case ACTIVE_FOR_EACH_INDEX_CLOSE:
            m_pActiveForEachIndex->Close();
            break;
          case ACTIVE_FOR_EACH_INDEX_REMOVE:
            for (size_t i = 0; i < m_Indexes.size(); ++i)
            {
              if (m_Indexes[i] == m_pActiveForEachIndex)
              {
                delete m_Indexes[i];
                m_Indexes.erase(m_Indexes.begin() + i);
                break;
              }
            }
            break;
          }
        }

        // Move iIndex to the next not visited index.
        if (iIndex >= m_Indexes.size())
          break;
        if (m_pActiveForEachIndex == m_Indexes[iIndex])
          ++iIndex;
        if (iIndex >= m_Indexes.size())
          break;

        m_pActiveForEachIndex = m_Indexes[iIndex];
        m_eActiveForEachIndexAction = ACTIVE_FOR_EACH_INDEX_DO_NOTHING;
      }

      if (IndexT * pIndex = m_pActiveForEachIndex->GetIndex(scale, occlusionRect))
        pIndex->ForEachInIntervalAndScale(f, beg, end, scale, query);
    }

    m_pActiveForEachIndex = NULL;
    m_eActiveForEachIndexAction = ACTIVE_FOR_EACH_INDEX_DO_NOTHING;
  }

  void Add(string const & path)
  {
    threads::MutexGuard mutexGuard(m_Mutex);
    UNUSED_VALUE(mutexGuard);

    for (size_t i = 0; i < m_Indexes.size(); ++i)
      if (m_Indexes[i]->IsMyData(path))
        return;

    m_Indexes.push_back(new IndexProxy(path));
  }

  void Remove(string const & path)
  {
    threads::MutexGuard mutexGuard(m_Mutex);
    UNUSED_VALUE(mutexGuard);

    for (size_t i = 0; i < m_Indexes.size(); ++i)
    {
      if (m_Indexes[i]->IsMyData(path))
      {
        if (m_Indexes[i] != m_pActiveForEachIndex)
        {
          delete m_Indexes[i];
          m_Indexes.erase(m_Indexes.begin() + i);
        }
        else
          m_eActiveForEachIndexAction = ACTIVE_FOR_EACH_INDEX_REMOVE;
        break;
      }
    }
  }

  void Clean()
  {
    threads::MutexGuard mutexGuard(m_Mutex);
    UNUSED_VALUE(mutexGuard);

    for (size_t i = 0; i < m_Indexes.size(); ++i)
    {
      if (m_Indexes[i] != m_pActiveForEachIndex)
        delete m_Indexes[i];
    }

    m_Indexes.clear();
    if (m_pActiveForEachIndex != NULL)
    {
      IndexProxy * pIndex = m_pActiveForEachIndex;
      m_Indexes.push_back(pIndex);
    }
  }

  void ClearCaches()
  {
    threads::MutexGuard mutexGuard(m_Mutex);
    UNUSED_VALUE(mutexGuard);

    for (size_t i = 0; i < m_Indexes.size(); ++i)
    {
      if (m_Indexes[i] != m_pActiveForEachIndex)
        m_Indexes[i]->Close();
      else
        if (m_eActiveForEachIndexAction != ACTIVE_FOR_EACH_INDEX_REMOVE)
          m_eActiveForEachIndexAction = ACTIVE_FOR_EACH_INDEX_CLOSE;
    }
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
        if (m_pIndex)
        {
          if (++m_QueriesSkipped > 8)
            Close();
        }
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

    void Close()
    {
      if (m_pIndex)
      {
        // LOG(LINFO, (m_Path));
        delete m_pIndex;
        m_pIndex = NULL;
        m_QueriesSkipped = 0;
      }
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

    m2::RectD m_Rect;
    string m_Path; // TODO: Store prefix and suffix of path in MultiIndexAdapter.
    IndexT * m_pIndex;
    uint8_t m_QueriesSkipped;
  };

  enum ActiveForEachIndexAction
  {
    ACTIVE_FOR_EACH_INDEX_DO_NOTHING = 0,
    ACTIVE_FOR_EACH_INDEX_CLOSE = 1,
    ACTIVE_FOR_EACH_INDEX_REMOVE = 2
  };

  mutable vector<IndexProxy *> m_Indexes;
  mutable IndexProxy * volatile m_pActiveForEachIndex;
  mutable ActiveForEachIndexAction volatile m_eActiveForEachIndexAction;
  mutable threads::Mutex m_Mutex;
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
  class Query : public BaseT::Query
  {
  public:
    // Clear query, so that it can be reused.
    // This function doesn't release caches!
    void Clear()
    {
      m_Offsets.clear();
      BaseT::Query::Clear();
    }

  private:
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
    BaseT::ForEachInIntervalAndScale(uniqueOffsetFunctorAdapter, beg, end, scale, query);
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
