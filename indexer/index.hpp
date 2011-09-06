#pragma once
#include "cell_id.hpp"
#include "covering.hpp"
#include "data_factory.hpp"
#include "features_vector.hpp"
#include "scale_index.hpp"
#include "search_trie.hpp"

#include "../../defines.hpp"

#include "../platform/platform.hpp"

#include "../geometry/rect2d.hpp"

#include "../coding/file_container.hpp"

#include "../base/base.hpp"
#include "../base/macros.hpp"
#include "../base/mutex.hpp"
#include "../base/stl_add.hpp"
#include "../base/thread.hpp"

#include "../std/algorithm.hpp"
#include "../std/bind.hpp"
#include "../std/string.hpp"
#include "../std/unordered_set.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"


template <class BaseT> class IndexForEachAdapter : public BaseT
{
private:
  template <typename F>
  void CallForIntervals(F & f, covering::IntervalsT const & intervals,
                        m2::RectD const & rect, uint32_t scale) const
  {
    for (size_t i = 0; i < intervals.size(); ++i)
    {
      BaseT::ForEachInIntervalAndScale(f, intervals[i].first, intervals[i].second,
                                       scale, rect);
    }
  }

public:
  template <typename F>
  void ForEachInRect(F & f, m2::RectD const & rect, uint32_t scale) const
  {
    CallForIntervals(f, covering::CoverViewportAndAppendLowerLevels(rect), rect, scale);
  }

  template <typename F>
  void ForEachInRect_TileDrawing(F & f, m2::RectD const & rect, uint32_t scale) const
  {
    using namespace covering;

    IntervalsT intervals;
    AppendLowerLevels(GetRectIdAsIs(rect), intervals);

    CallForIntervals(f, intervals, rect, scale);
  }

public:
  template <typename F>
  void ForEachInScale(F & f, uint32_t scale) const
  {
    int64_t const rootId = RectId("").ToInt64();
    BaseT::ForEachInIntervalAndScale(f, rootId, rootId + RectId("").SubTreeSize(), scale,
                                     m2::RectD::GetInfiniteRect());
  }
};

template <class IndexT> class MultiIndexAdapter
{
public:
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
  void ForEachInIntervalAndScale(F & f, int64_t beg, int64_t end, uint32_t scale,
                                 m2::RectD const & occlusionRect) const
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
          (void)UpdateIndex(iIndex);
        }
        else
        {
          pProxy = m_indexes[iIndex];
          pIndex = pProxy->Lock(static_cast<int>(scale), occlusionRect);
          ++iIndex;
        }
      }

      if (pIndex) // pIndex may be NULL because it doesn't match scale or occlusionRect.
      {
        ProxyUnlockGuard proxyUnlockGuard(m_mutex, pProxy);
        UNUSED_VALUE(proxyUnlockGuard);
        pIndex->ForEachInIntervalAndScale(f, beg, end, scale);
      }
    }
  }

  search::SearchInfo * GetWorldSearchInfo() const
  {
    return m_pWorldSearchInfo.get();
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

    //if (m_indexes.back()->IsWorldData())
    //{
    //  ASSERT ( !m_pWorldSearchInfo.get(), () );
    //  m_pWorldSearchInfo.reset(m_indexes.back()->GetSearchInfo());
    //}
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
      : m_action(INDEX_DO_NOTHING), m_file(file), m_lockCount(0),
        m_queriesSkipped(0)
    {
      m_factory.Load(FilesContainerR(GetPlatform().GetReader(m_file)));

      feature::DataHeader const & h = m_factory.GetHeader();
      m_rect = h.GetBounds();
      m_scaleRange = h.GetScaleRange();
    }

    IndexT * Lock(int scale, m2::RectD const & occlusionRect)
    {
      IndexT * & p = GetThreadIndex();

      if ((m_scaleRange.first <= scale && scale <= m_scaleRange.second) &&
          m_rect.IsIntersect(occlusionRect))
      {
        Open(p);
        m_queriesSkipped = 0;
        ++m_lockCount;
        return p;
      }
      else
      {
        if (p)
        {
          if (++m_queriesSkipped > 8)
            Close(p);
        }
        return NULL;
      }
    }

    void Unlock()
    {
      ASSERT_GREATER(m_lockCount, 0, ());
      ASSERT(GetThreadIndex(), ());
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

    bool IsWorldData() const
    {
      return m_scaleRange.first <= 1;
    }

    search::SearchInfo * GetSearchInfo() const
    {
      return new search::SearchInfo(FilesContainerR(GetPlatform().GetReader(m_file)),
                                    m_factory.GetHeader());
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
      IndexProxy * pRes = new IndexProxy(m_file);
      return pRes;
    }

    volatile IndexAction m_action;

  private:
    void Open(IndexT * & p)
    {
      if (p == 0)
      {
        p = new IndexT(FilesContainerR(GetPlatform().GetReader(m_file)), m_factory);
      }
    }

    void Close(IndexT * & p)
    {
      if (p)
      {
        // LOG(LINFO, (m_Path));
        delete p;
        p = NULL;
        m_queriesSkipped = 0;
      }
    }
    void Close()
    {
      Close(GetThreadIndex());
    }

    IndexT * & GetThreadIndex()
    {
      return m_indexes[threads::GetCurrentThreadID()];
    }

    string m_file;

    IndexFactory m_factory;
    m2::RectD m_rect;
    pair<int, int> m_scaleRange;

    map<threads::ThreadID, IndexT *> m_indexes;
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
  scoped_ptr<search::SearchInfo> m_pWorldSearchInfo;
};

template <class FeatureVectorT, class BaseT> class OffsetToFeatureAdapter : public BaseT
{
public:
  OffsetToFeatureAdapter(FilesContainerR const & cont, IndexFactory & factory)
  : BaseT(cont.GetReader(INDEX_FILE_TAG), factory),
    m_FeatureVector(cont, factory.GetHeader())
  {
  }

  template <typename F>
  void ForEachInIntervalAndScale(F & f, int64_t beg, int64_t end, uint32_t scale) const
  {
    OffsetToFeatureReplacer<F> offsetToFeatureReplacer(m_FeatureVector, f);
    BaseT::ForEachInIntervalAndScale(offsetToFeatureReplacer, beg, end, scale);
  }

private:
  FeatureVectorT m_FeatureVector;

  template <typename F>
  class OffsetToFeatureReplacer
  {
    FeatureVectorT const & m_V;
    F & m_F;

  public:
    OffsetToFeatureReplacer(FeatureVectorT const & v, F & f) : m_V(v), m_F(f) {}
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
  template <typename T1>
  explicit UniqueOffsetAdapter(T1 const & t1) : BaseT(t1) {}

  template <typename T1, typename T2>
  UniqueOffsetAdapter(T1 const & t1, T2 & t2) : BaseT(t1, t2) {}

  template <typename F>
  void ForEachInIntervalAndScale(F & f, int64_t beg, int64_t end, uint32_t scale) const
  {
    unordered_set<uint32_t> offsets;
    UniqueOffsetFunctorAdapter<F> uniqueOffsetFunctorAdapter(offsets, f);
    BaseT::ForEachInIntervalAndScale(uniqueOffsetFunctorAdapter, beg, end, scale);
  }

private:
  template <typename F>
  struct UniqueOffsetFunctorAdapter
  {
    UniqueOffsetFunctorAdapter(unordered_set<uint32_t> & offsets, F & f)
      : m_Offsets(offsets), m_F(f) {}

    void operator() (uint32_t offset) const
    {
      if (m_Offsets.insert(offset).second)
        m_F(offset);
    }

    unordered_set<uint32_t> & m_Offsets;
    F & m_F;
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
