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

  explicit MwmValue(string const & name);

  inline feature::DataHeader const & GetHeader() const { return m_factory.GetHeader(); }
  inline version::MwmVersion const & GetMwmVersion() const { return m_factory.GetMwmVersion(); }

  /// @return MWM file name without extension.
  string GetFileName() const;
};

class Index : public MwmSet
{
protected:
  // MwmSet overrides:
  bool GetVersion(string const & name, MwmInfo & info) const override;
  TMwmValueBasePtr CreateValue(string const & name) const override;
  void OnMwmDeleted(shared_ptr<MwmInfo> const & info) override;
  void OnMwmReadyForUpdate(shared_ptr<MwmInfo> const & info) override;

public:
  enum UpdateStatus
  {
    UPDATE_STATUS_OK,
    UPDATE_STATUS_BAD_FILE,
    UPDATE_STATUS_UPDATE_DELAYED
  };

  /// An Observer interface to MwmSet. Note that these functions can
  /// be called from *ANY* thread because most signals are sent when
  /// some thread releases its MwmLock, so overrides must be as fast
  /// as possible and non-blocking when it's possible.
  class Observer
  {
  public:
    virtual ~Observer() {}

    /// Called when a map is registered for a first time.
    virtual void OnMapRegistered(string const & file) {}

    /// Called when an update for a map is downloaded.
    virtual void OnMapUpdateIsReady(string const & file) {}

    /// Called when an update for a map is applied.
    virtual void OnMapUpdated(string const & file) {}

    /// Called when a map is deleted.
    virtual void OnMapDeleted(string const & file) {}
  };

  Index();
  ~Index() override;

  /// Registers a new map.
  ///
  /// \return A pair of an MwmLock and a flag. MwmLock is locked iff the
  ///         map with fileName was created or already exists. Flag
  ///         is set when the map was registered for a first
  ///         time. Thus, there are three main cases:
  ///
  ///         * the map already exists - returns active lock and unset flag
  ///         * the map was already registered - returns active lock and set flag
  ///         * the map can't be registered - returns inactive lock and unset flag
  WARN_UNUSED_RESULT pair<MwmLock, bool> RegisterMap(string const & fileName);

  /// Replaces a map file corresponding to fileName with a new one, when
  /// it's possible - no clients of the map file. Otherwise, update
  /// will be delayed.
  ///
  /// \return * the map file have been updated - returns active lock and
  ///           UPDATE_STATUS_OK
  ///         * update is delayed because the map is busy - returns active lock and
  ///           UPDATE_STATUS_UPDATE_DELAYED
  ///         * the file isn't suitable for update - returns inactive lock and
  ///           UPDATE_STATUS_BAD_FILE
  WARN_UNUSED_RESULT pair<MwmLock, UpdateStatus> UpdateMap(string const & fileName);

  /// Deletes a map both from the file system and internal tables, also,
  /// deletes all files related to the map. If the map was successfully
  /// deleted, notifies observers.
  ///
  /// \param fileName A fileName denoting the map to be deleted, may
  ///                 be a full path or a short path relative to
  ///                 executable's directories.
  //// \return True if the map was successfully deleted.
  bool DeleteMap(string const & fileName);

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
    string GetFileName() const;
    bool IsWorld() const;
    void GetFeature(uint32_t offset, FeatureType & ft);

  private:
    MwmLock m_lock;
    FeaturesVector m_vector;
  };

  MwmId GetMwmIdByName(string const & name) const
  {
    lock_guard<mutex> lock(m_lock);

    MwmId const id = GetIdByName(name);
    ASSERT(id.IsAlive(), ("Can't get an mwm's identifier."));
    return id;
  }

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
      FeatureID fakeID(id, numeric_limits<uint32_t>::max());
      result = distance(features.begin(), upper_bound(features.begin(), features.end(), fakeID));
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
