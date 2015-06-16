#pragma once
#include "indexer/cell_id.hpp"
#include "indexer/data_factory.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/scale_index.hpp"

#include "coding/file_container.hpp"

#include "defines.hpp"

#include "base/macros.hpp"
#include "base/observer_list.hpp"

#include "std/algorithm.hpp"
#include "std/limits.hpp"
#include "std/unordered_set.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

class MwmValue : public MwmSet::MwmValueBase
{
public:
  FilesContainerR m_cont;
  IndexFactory m_factory;
  platform::CountryFile const m_countryFile;

  explicit MwmValue(platform::LocalCountryFile const & localFile);

  inline feature::DataHeader const & GetHeader() const { return m_factory.GetHeader(); }
  inline version::MwmVersion const & GetMwmVersion() const { return m_factory.GetMwmVersion(); }

  inline platform::CountryFile const & GetCountryFile() const { return m_countryFile; }
};

class Index : public MwmSet
{
protected:
  // MwmSet overrides:
  bool GetVersion(platform::LocalCountryFile const & localFile, MwmInfo & info) const override;
  TMwmValueBasePtr CreateValue(platform::LocalCountryFile const & localFile) const override;
  void OnMwmDeregistered(platform::LocalCountryFile const & localFile) override;

public:
  /// An Observer interface to MwmSet. Note that these functions can
  /// be called from *ANY* thread because most signals are sent when
  /// some thread releases its MwmLock, so overrides must be as fast
  /// as possible and non-blocking when it's possible.
  class Observer
  {
  public:
    virtual ~Observer() = default;

    /// Called when a map is registered for a first time and can be
    /// used.
    virtual void OnMapRegistered(platform::LocalCountryFile const & localFile) {}

    /// Called when a map is deregistered and can not be used.
    virtual void OnMapDeregistered(platform::LocalCountryFile const & localFile) {}
  };

  Index();
  ~Index() override;

  /// Registers a new map.
  ///
  /// \return A pair of an MwmLock and a flag. There are three cases:
  ///         * the map is newer than the newset registered - returns
  ///           active lock and set flag
  ///         * the map is older than the newset registered - returns inactive lock and
  ///           unset flag.
  ///         * the version of the map equals to the version of the newest registered -
  ///           returns active lock and unset flag.
  WARN_UNUSED_RESULT pair<MwmLock, bool> RegisterMap(platform::LocalCountryFile const & localFile);

  /// Deregisters a map from internal records.
  ///
  /// \param countryFile A countryFile denoting a map to be deregistered.
  /// \return True if the map was successfully deregistered. If map is locked
  ///         now, returns false.
  bool DeregisterMap(platform::CountryFile const & countryFile);

  bool AddObserver(Observer & observer);

  bool RemoveObserver(Observer const & observer);

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
      MwmValue * const pValue = lock.GetValue<MwmValue>();
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
        ImplFunctor implF(fv, m_f, lock.GetId());
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
        ASSERT(m_id.IsAlive(), ());
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
      MwmValue * const pValue = lock.GetValue<MwmValue>();
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
        ImplFunctor implF(m_f, lock.GetId());
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
  public:
    FeaturesLoaderGuard(Index const & parent, MwmId id);

    inline MwmSet::MwmId GetId() const { return m_lock.GetId(); }
    string GetCountryFileName() const;
    bool IsWorld() const;
    void GetFeature(uint32_t offset, FeatureType & ft);

  private:
    MwmLock m_lock;
    FeaturesVector m_vector;
  };

  template <typename F>
  void ForEachInRectForMWM(F & f, m2::RectD const & rect, uint32_t scale, MwmId const id) const
  {
    if (id.IsAlive())
    {
      MwmLock const lock(const_cast<Index &>(*this), id);
      if (lock.IsLocked())
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
    MwmId id = features[index].m_mwmId;
    MwmLock const lock(const_cast<Index &>(*this), id);
    MwmValue * const pValue = lock.GetValue<MwmValue>();
    if (pValue)
    {
      FeaturesVector featureReader(pValue->m_cont, pValue->GetHeader());
      while (result < features.size() && id == features[result].m_mwmId)
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
      // Fake feature identifier which is used to determine the right
      // bound of all features in an mwm corresponding to id, because
      // it's greater than any other feature in the mwm in accordance
      // with FeatureID::operator<().
      FeatureID const fakeID(id, numeric_limits<uint32_t>::max());
      result = distance(features.cbegin(), upper_bound(features.cbegin(), features.cend(), fakeID));
    }

    return result;
  }

  template <typename F>
  void ForEachInIntervals(F & f, covering::CoveringMode mode, m2::RectD const & rect,
                          uint32_t scale) const
  {
    vector<shared_ptr<MwmInfo>> mwms;
    GetMwmsInfo(mwms);

    covering::CoveringGetter cov(rect, mode);

    MwmId worldID[2];

    for (shared_ptr<MwmInfo> const & info : mwms)
    {
      if (info->m_minScale <= scale && scale <= info->m_maxScale &&
          rect.IsIntersect(info->m_limitRect))
      {
        MwmId id(info);
        switch (info->GetType())
        {
          case MwmInfo::COUNTRY:
          {
            MwmLock const lock(const_cast<Index &>(*this), id);
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

    if (worldID[0].IsAlive())
    {
      MwmLock const lock(const_cast<Index &>(*this), worldID[0]);
      f(lock, cov, scale);
    }

    if (worldID[1].IsAlive())
    {
      MwmLock const lock(const_cast<Index &>(*this), worldID[1]);
      f(lock, cov, scale);
    }
  }

  my::ObserverList<Observer> m_observers;
};
