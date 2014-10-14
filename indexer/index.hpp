#pragma once
#include "cell_id.hpp"
#include "data_factory.hpp"
#include "mwm_set.hpp"
#include "feature_covering.hpp"
#include "features_vector.hpp"
#include "scale_index.hpp"

#include "../coding/file_container.hpp"

#include "../defines.hpp"

#include "../std/vector.hpp"
#include "../std/unordered_set.hpp"
#include "../std/algorithm.hpp"


class MwmValue : public MwmSet::MwmValueBase
{
public:
  FilesContainerR m_cont;
  IndexFactory m_factory;

  explicit MwmValue(string const & name);

  inline feature::DataHeader const & GetHeader() const { return m_factory.GetHeader(); }

  /// @return MWM file name without extension.
  string GetFileName() const;
};


class Index : public MwmSet
{
protected:
  /// @return mwm format version
  virtual int GetInfo(string const & name, MwmInfo & info) const;
  virtual MwmValue * CreateValue(string const & name) const;
  virtual void UpdateMwmInfo(MwmId id);

public:
  Index();
  ~Index();

  class MwmLock : public MwmSet::MwmLock
  {
    typedef MwmSet::MwmLock BaseT;
  public:
    MwmLock(Index const & index, MwmId mwmId)
      : BaseT(const_cast<Index &>(index), mwmId) {}

    inline MwmValue * GetValue() const
    {
      return static_cast<MwmValue *>(BaseT::GetValue());
    }

    /// @return MWM file name without extension.
    /// If value is 0, an empty string returned.
    string GetFileName() const;
  };

  bool DeleteMap(string const & fileName);
  bool UpdateMap(string const & fileName, m2::RectD & rect);

private:

  template <typename F>
  class ReadMWMFunctor
  {
    class ImplFunctor : private noncopyable
    {
      FeaturesVector const & m_V;
      F & m_F;
      unordered_set<uint32_t> m_offsets;
      MwmId m_mwmID;

    public:
      ImplFunctor(FeaturesVector const & v, F & f, MwmId mwmID)
        : m_V(v), m_F(f), m_mwmID(mwmID)
      {
      }

      void operator() (uint32_t offset)
      {
        if (m_offsets.insert(offset).second)
        {
          FeatureType feature;

          m_V.Get(offset, feature);
          feature.SetID(FeatureID(m_mwmID, offset));

          m_F(feature);
        }
      }
    };

  public:
    ReadMWMFunctor(F & f) : m_f(f) {}

    void operator() (MwmLock const & lock, covering::CoveringGetter & cov, uint32_t scale) const
    {
      MwmValue * pValue = lock.GetValue();
      if (pValue)
      {
        feature::DataHeader const & header = pValue->GetHeader();

        // Prepare needed covering.
        uint32_t const lastScale = header.GetLastScale();

        // In case of WorldCoasts we should pass correct scale in ForEachInIntervalAndScale.
        if (scale > lastScale) scale = lastScale;

        // Use last coding scale for covering (see index_builder.cpp).
        covering::IntervalsT const & interval = cov.Get(lastScale);

        // prepare features reading
        FeaturesVector fv(pValue->m_cont, header);
        ScaleIndex<ModelReaderPtr> index(pValue->m_cont.GetReader(INDEX_FILE_TAG),
                                         pValue->m_factory);

        // iterate through intervals
        ImplFunctor implF(fv, m_f, lock.GetID());
        for (size_t i = 0; i < interval.size(); ++i)
          index.ForEachInIntervalAndScale(implF, interval[i].first, interval[i].second, scale);
      }
    }

  private:
    F & m_f;
  };

  template <typename F>
  class ReadFeatureIndexFunctor
  {
    struct ImplFunctor : private noncopyable
    {
    public:
      ImplFunctor(F & f, MwmId id) : m_f(f), m_id(id) {}

      void operator() (uint32_t offset)
      {
        ASSERT(m_id != -1, ());
        if (m_offsets.insert(offset).second)
          m_f(FeatureID(m_id, offset));
      }

    private:
      F & m_f;
      MwmId m_id;
      unordered_set<uint32_t> m_offsets;
    };

  public:
    ReadFeatureIndexFunctor(F & f) : m_f(f) {}

    void operator() (MwmLock const & lock, covering::CoveringGetter & cov, uint32_t scale) const
    {
      MwmValue * pValue = lock.GetValue();
      if (pValue)
      {
        feature::DataHeader const & header = pValue->GetHeader();

        // Prepare needed covering.
        int const lastScale = header.GetLastScale();

        // In case of WorldCoasts we should pass correct scale in ForEachInIntervalAndScale.
        if (scale > lastScale) scale = lastScale;

        // Use last coding scale for covering (see index_builder.cpp).
        covering::IntervalsT const & interval = cov.Get(lastScale);
        ScaleIndex<ModelReaderPtr> index(pValue->m_cont.GetReader(INDEX_FILE_TAG),
                                         pValue->m_factory);

        // iterate through intervals
        ImplFunctor implF(m_f, lock.GetID());
        for (size_t i = 0; i < interval.size(); ++i)
          index.ForEachInIntervalAndScale(implF, interval[i].first, interval[i].second, scale);
      }
    }

  private:
    F & m_f;
  };

public:

  template <typename F>
  void ForEachInRect(F & f, m2::RectD const & rect, uint32_t scale) const
  {
    ReadMWMFunctor<F> implFunctor(f);
    ForEachInIntervals(implFunctor, covering::ViewportWithLowLevels, rect, scale);
  }

  template <typename F>
  void ForEachInRect_TileDrawing(F & f, m2::RectD const & rect, uint32_t scale) const
  {
    ReadMWMFunctor<F> implFunctor(f);
    ForEachInIntervals(implFunctor, covering::LowLevelsOnly, rect, scale);
  }

  template <typename F>
  void ForEachFeatureIDInRect(F & f, m2::RectD const & rect, uint32_t scale) const
  {
    ReadFeatureIndexFunctor<F> implFunctor(f);
    ForEachInIntervals(implFunctor, covering::LowLevelsOnly, rect, scale);
  }

  template <typename F>
  void ForEachInScale(F & f, uint32_t scale) const
  {
    ReadMWMFunctor<F> implFunctor(f);
    ForEachInIntervals(implFunctor, covering::FullCover, m2::RectD::GetInfiniteRect(), scale);
  }

  // "features" must be sorted using FeatureID::operator< as predicate
  template <typename F>
  void ReadFeatures(F & f, vector<FeatureID> const & features) const
  {
    size_t currentIndex = 0;
    while (currentIndex < features.size())
      currentIndex = ReadFeatureRange(f, features, currentIndex);
  }

  /// Guard for loading features from particular MWM by demand.
  class FeaturesLoaderGuard
  {
    MwmLock m_lock;
    FeaturesVector m_vector;

  public:
    FeaturesLoaderGuard(Index const & parent, MwmId id);

    inline MwmSet::MwmId GetID() const { return m_lock.GetID(); }
    inline string GetFileName() const { return m_lock.GetFileName(); }

    bool IsWorld() const;
    void GetFeature(uint32_t offset, FeatureType & ft);
  };

  template <typename F>
  void ForEachInRectForMWM(F & f, m2::RectD const & rect, uint32_t scale, string const & name) const
  {
    MwmId id;
    {
      Index * p = const_cast<Index *>(this);

      threads::MutexGuard guard(p->m_lock);
      UNUSED_VALUE(guard);
      id = p->GetIdByName(name);
    }

    if (id != INVALID_MWM_ID)
    {
      MwmLock lock(*this, id);
      if (lock.GetValue())
      {
        covering::CoveringGetter cov(rect, covering::ViewportWithLowLevels);
        ReadMWMFunctor<F> fn(f);
        fn(lock, cov, scale);
      }
    }
  }

private:

  // "features" must be sorted using FeatureID::operator< as predicate
  template <typename F>
  size_t ReadFeatureRange(F & f, vector<FeatureID> const & features, size_t index) const
  {
    ASSERT_LESS(index, features.size(), ());
    size_t result = index;
    MwmId id = features[index].m_mwm;
    MwmLock lock(*this, id);
    MwmValue * pValue = lock.GetValue();
    if (pValue)
    {
      FeaturesVector featureReader(pValue->m_cont, pValue->GetHeader());
      while (result < features.size() && id == features[result].m_mwm)
      {
        FeatureID const & featureId = features[result];
        FeatureType featureType;

        featureReader.Get(featureId.m_offset, featureType);
        featureType.SetID(featureId);

        f(featureType);
        ++result;
      }
    }
    else
    {
      result = distance(features.begin(),
                        lower_bound(features.begin(), features.end(), FeatureID(id + 1, 0)));
    }

    return result;
  }

  template <typename F>
  void ForEachInIntervals(F & f, covering::CoveringMode mode,
                          m2::RectD const & rect, uint32_t scale) const
  {
    vector<MwmInfo> mwm;
    GetMwmInfo(mwm);

    covering::CoveringGetter cov(rect, mode);

    size_t const count = mwm.size();
    MwmId worldID[2] = { count, count };

    for (MwmId id = 0; id < count; ++id)
    {
      if ((mwm[id].m_minScale <= scale && scale <= mwm[id].m_maxScale) &&
          rect.IsIntersect(mwm[id].m_limitRect))
      {
        switch (mwm[id].GetType())
        {
        case MwmInfo::COUNTRY:
          {
            MwmLock lock(*this, id);
            f(lock, cov, scale);
          }
          break;

        case MwmInfo::COASTS:
          worldID[0] = id;
          break;

        case MwmInfo::WORLD:
          worldID[1] = id;
          break;
        }
      }
    }

    if (worldID[0] < count)
    {
      MwmLock lock(*this, worldID[0]);
      f(lock, cov, scale);
    }

    if (worldID[1] < count)
    {
      MwmLock lock(*this, worldID[1]);
      f(lock, cov, scale);
    }
  }
};
