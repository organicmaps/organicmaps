#pragma once
#include "cell_id.hpp"
#include "covering.hpp"
#include "data_header.hpp"
#include "features_vector.hpp"
#include "scale_index.hpp"
#include "scales.hpp"

#include "../../defines.hpp"

#include "../platform/platform.hpp"

#include "../geometry/rect2d.hpp"

#include "../coding/file_container.hpp"

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

  MultiIndexAdapter()
  {
  }

  MultiIndexAdapter(MultiIndexAdapter const & index) : m_indexes(index.m_indexes.size())
  {
    for (size_t i = 0; i < index.m_indexes.size(); ++i)
    {
      CHECK(index.m_indexes[i], ());
      m_indexes[i] = index.m_indexes[i]->Clone();
    }
  }

  ~MultiIndexAdapter()
  {
    Clean();
    ASSERT_EQUAL(m_indexes.size(), 0, ());
  }

  template <typename F>
  void ForEachInIntervalAndScale(F const & f, int64_t beg, int64_t end, uint32_t scale,
                                 m2::RectD const & occlusionRect, Query & query) const
  {
    for (size_t iIndex = 0; true;)
    {
      IndexT * pIndex = NULL;
      IndexProxy * pProxy = NULL;
      {
        threads::MutexGuard mutexGuard(m_mutex);
        UNUSED_VALUE(mutexGuard);

        if (iIndex >= m_indexes.size())
          break;

        if (m_indexes[iIndex]->m_action == IndexProxy::INDEX_REMOVE)
        {
          UpdateIndex(iIndex);
        }
        else
        {
          pProxy = m_indexes[iIndex];
          pIndex = pProxy->Lock(scale, occlusionRect);
          ++iIndex;
        }
      }

      if (pIndex) // pIndex may be NULL because it doesn't match scale or occlusionRect.
      {
        ProxyUnlockGuard proxyUnlockGuard(m_mutex, pProxy);
        UNUSED_VALUE(proxyUnlockGuard);
        pIndex->ForEachInIntervalAndScale(f, beg, end, scale, query);
      }
    }
  }

  void Add(string const & file)
  {
    threads::MutexGuard mutexGuard(m_mutex);
    UNUSED_VALUE(mutexGuard);

    for (size_t i = 0; i < m_indexes.size(); ++i)
      if (m_indexes[i]->IsMyData(file))
        return;

    m_indexes.push_back(new IndexProxy(file));

    UpdateIndexes();
  }

  void Remove(string const & file)
  {
    threads::MutexGuard mutexGuard(m_mutex);
    UNUSED_VALUE(mutexGuard);

    for (size_t i = 0; i < m_indexes.size(); ++i)
    {
      if (m_indexes[i]->IsMyData(file))
        m_indexes[i]->m_action = IndexProxy::INDEX_REMOVE;
    }

    UpdateIndexes();
  }

  // Remove all indexes.
  void Clean()
  {
    threads::MutexGuard mutexGuard(m_mutex);
    UNUSED_VALUE(mutexGuard);

    for (size_t i = 0; i < m_indexes.size(); ++i)
      m_indexes[i]->m_action = IndexProxy::INDEX_REMOVE;

    UpdateIndexes();
  }

  // Close all indexes.
  void ClearCaches()
  {
    threads::MutexGuard mutexGuard(m_mutex);
    UNUSED_VALUE(mutexGuard);

    for (size_t i = 0; i < m_indexes.size(); ++i)
      m_indexes[i]->m_action = IndexProxy::INDEX_CLOSE;

    UpdateIndexes();
  }

private:

  // Updates m_index[i]. Returns true, if index wasn't removed.
  bool UpdateIndex(size_t i) const
  {
    if (!m_indexes[i]->IsUnlocked())
      return true;

    if (m_indexes[i]->m_action == IndexProxy::INDEX_REMOVE)
    {
      delete m_indexes[i];
      m_indexes.erase(m_indexes.begin() + i);
      return false;
    }

    if (m_indexes[i]->m_action == IndexProxy::INDEX_CLOSE)
      m_indexes[i]->CloseIfUnlocked();

    return true;
  }

  // Updates all indexes.
  void UpdateIndexes() const
  {
    for (size_t i = 0; i < m_indexes.size(); )
      if (UpdateIndex(i))
        ++i;
  }

  class IndexProxy
  {
  public:
    explicit IndexProxy(string const & file)
      : m_action(INDEX_DO_NOTHING), m_file(file), m_pIndex(NULL), m_lockCount(0),
        m_queriesSkipped(0)
    {
      feature::DataHeader header;
      header.Load(FilesContainerR(GetPlatform().GetReader(m_file)).GetReader(HEADER_FILE_TAG));

      m_rect = header.GetBounds();
      m_scaleRange = header.GetScaleRange();
    }

    IndexT * Lock(uint32_t scale, m2::RectD const & occlusionRect)
    {
      if ((m_scaleRange.first <= scale && scale <= m_scaleRange.second) &&
          m_rect.IsIntersect(occlusionRect))
      {
        Open();
        m_queriesSkipped = 0;
        ++m_lockCount;
        return m_pIndex;
      }
      else
      {
        if (m_pIndex)
        {
          if (++m_queriesSkipped > 8)
            Close();
        }
        return NULL;
      }
    }

    void Unlock()
    {
      ASSERT_GREATER(m_lockCount, 0, ());
      ASSERT(m_pIndex, ());
      if (m_lockCount > 0)
        --m_lockCount;
    }

    bool IsUnlocked() const
    {
      return m_lockCount == 0;
    }

    bool IsMyData(string const & file) const
    {
      return Reader::IsEqual(m_file, file);
    }

    void CloseIfUnlocked()
    {
      if (IsUnlocked())
        Close();
    }

    ~IndexProxy()
    {
      ASSERT_EQUAL(m_lockCount, 0, ());
      Close();
    }

    enum IndexAction
    {
      INDEX_DO_NOTHING = 0,
      INDEX_CLOSE = 1,
      INDEX_REMOVE = 2
    };

    IndexProxy * Clone() const
    {
      IndexProxy * pRes = new IndexProxy(*this);
      pRes->m_action = INDEX_DO_NOTHING;
      pRes->m_pIndex = NULL;
      pRes->m_lockCount = 0;
      pRes->m_queriesSkipped = 0;
      return pRes;
    }

    volatile IndexAction m_action;

  private:

    void Open()
    {
      if (!m_pIndex)
      {
        FilesContainerR container(GetPlatform().GetReader(m_file));
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
        m_queriesSkipped = 0;
      }
    }

    string m_file;
    m2::RectD m_rect;
    pair<int, int> m_scaleRange;

    IndexT * volatile m_pIndex;
    uint16_t volatile m_lockCount;
    uint8_t volatile m_queriesSkipped;
  };

  // Helper class that unlocks given IndexProxy in destructor.
  class ProxyUnlockGuard
  {
    threads::Mutex & m_mutex;
    IndexProxy * m_pProxy;

  public:
    ProxyUnlockGuard(threads::Mutex & mutex, IndexProxy * pProxy)
      : m_mutex(mutex), m_pProxy(pProxy)
    {
    }

    ~ProxyUnlockGuard()
    {
      threads::MutexGuard mutexGuard(m_mutex);
      UNUSED_VALUE(mutexGuard);

      m_pProxy->Unlock();

      if (m_pProxy->m_action == IndexProxy::INDEX_CLOSE)
        m_pProxy->CloseIfUnlocked();
    }
  };

  mutable vector<IndexProxy *> m_indexes;
  mutable threads::Mutex m_mutex;
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

  //bool IsMyData(string const & fName) const
  //{
  //  return m_FeatureVector.IsMyData(fName);
  //}

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
