#pragma once
#include "indexer/cell_id.hpp"
#include "indexer/data_factory.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/scale_index.hpp"
#include "indexer/unique_index.hpp"

#include "coding/file_container.hpp"

#include "defines.hpp"

#include "base/macros.hpp"
#include "base/observer_list.hpp"

#include "std/algorithm.hpp"
#include "std/limits.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"


class MwmInfoEx : public MwmInfo
{
public:
  unique_ptr<feature::FeaturesOffsetsTable> m_table;
};

class MwmValue : public MwmSet::MwmValueBase
{
public:
  FilesContainerR const m_cont;
  IndexFactory m_factory;
  platform::LocalCountryFile const m_file;
  feature::FeaturesOffsetsTable const * m_table;

  explicit MwmValue(platform::LocalCountryFile const & localFile);
  void SetTable(MwmInfoEx & info);

  inline feature::DataHeader const & GetHeader() const { return m_factory.GetHeader(); }
  inline version::MwmVersion const & GetMwmVersion() const { return m_factory.GetMwmVersion(); }

  inline platform::CountryFile const & GetCountryFile() const { return m_file.GetCountryFile(); }
};

class Index : public MwmSet
{
protected:
  /// @name MwmSet overrides.
  //@{
  MwmInfoEx * CreateInfo(platform::LocalCountryFile const & localFile) const override;

  MwmValue * CreateValue(MwmInfo & info) const override;

  void OnMwmDeregistered(platform::LocalCountryFile const & localFile) override;
  //@}

public:
  /// An Observer interface to MwmSet. Note that these functions can
  /// be called from *ANY* thread because most signals are sent when
  /// some thread releases its MwmHandle, so overrides must be as fast
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


  /// Registers a new map.
  WARN_UNUSED_RESULT pair<MwmHandle, RegResult> RegisterMap(
      platform::LocalCountryFile const & localFile);

  /// Deregisters a map from internal records.
  ///
  /// \param countryFile A countryFile denoting a map to be deregistered.
  /// \return True if the map was successfully deregistered. If map is locked
  ///         now, returns false.
  bool DeregisterMap(platform::CountryFile const & countryFile);

  bool AddObserver(Observer & observer);

  bool RemoveObserver(Observer const & observer);

private:

  template <typename F> class ReadMWMFunctor
  {
    F & m_f;
  public:
    ReadMWMFunctor(F & f) : m_f(f) {}

    void operator()(MwmHandle const & handle, covering::CoveringGetter & cov, uint32_t scale) const
    {
      MwmValue * const pValue = handle.GetValue<MwmValue>();
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
        FeaturesVector fv(pValue->m_cont, header, pValue->m_table);
        ScaleIndex<ModelReaderPtr> index(pValue->m_cont.GetReader(INDEX_FILE_TAG),
                                         pValue->m_factory);

        // iterate through intervals
        CheckUniqueIndexes checkUnique(header.GetFormat() >= version::v5);
        MwmId const mwmID = handle.GetId();

        for (auto const & i : interval)
        {
          index.ForEachInIntervalAndScale([&] (uint32_t index)
          {
            if (checkUnique(index))
            {
              FeatureType feature;

              fv.GetByIndex(index, feature);
              feature.SetID(FeatureID(mwmID, index));

              m_f(feature);
            }
          }, i.first, i.second, scale);
        }
      }
    }
  };

  template <typename F> class ReadFeatureIndexFunctor
  {
    F & m_f;
  public:
    ReadFeatureIndexFunctor(F & f) : m_f(f) {}

    void operator()(MwmHandle const & handle, covering::CoveringGetter & cov, uint32_t scale) const
    {
      MwmValue * const pValue = handle.GetValue<MwmValue>();
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
        CheckUniqueIndexes checkUnique(header.GetFormat() >= version::v5);
        MwmId const mwmID = handle.GetId();

        for (auto const & i : interval)
        {
          index.ForEachInIntervalAndScale([&] (uint32_t index)
          {
            if (checkUnique(index))
              m_f(FeatureID(mwmID, index));
          }, i.first, i.second, scale);
        }
      }
    }
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

    inline MwmSet::MwmId GetId() const { return m_handle.GetId(); }
    string GetCountryFileName() const;
    bool IsWorld() const;
    void GetFeatureByIndex(uint32_t index, FeatureType & ft);

  private:
    MwmHandle m_handle;
    FeaturesVector m_vector;
  };

  template <typename F>
  void ForEachInRectForMWM(F & f, m2::RectD const & rect, uint32_t scale, MwmId const id) const
  {
    if (id.IsAlive())
    {
      MwmHandle const handle(const_cast<Index &>(*this), id);
      if (handle.IsAlive())
      {
        covering::CoveringGetter cov(rect, covering::ViewportWithLowLevels);
        ReadMWMFunctor<F> fn(f);
        fn(handle, cov, scale);
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
    MwmHandle const handle(const_cast<Index &>(*this), id);
    MwmValue * const pValue = handle.GetValue<MwmValue>();
    if (pValue)
    {
      FeaturesVector featureReader(pValue->m_cont, pValue->GetHeader(), pValue->m_table);
      while (result < features.size() && id == features[result].m_mwmId)
      {
        FeatureID const & featureId = features[result];
        FeatureType featureType;

        featureReader.GetByIndex(featureId.m_index, featureType);
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
            MwmHandle const handle(const_cast<Index &>(*this), id);
            f(handle, cov, scale);
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
      MwmHandle const handle(const_cast<Index &>(*this), worldID[0]);
      f(handle, cov, scale);
    }

    if (worldID[1].IsAlive())
    {
      MwmHandle const handle(const_cast<Index &>(*this), worldID[1]);
      f(handle, cov, scale);
    }
  }

  my::ObserverList<Observer> m_observers;
};
