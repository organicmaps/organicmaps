#pragma once

#include "indexer/cell_id.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/feature_source.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/scale_index.hpp"
#include "indexer/unique_index.hpp"

#include "coding/file_container.hpp"

#include "base/macros.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "defines.hpp"

template <typename FeatureSource, typename Element>
class ReadMWMFunctor final
{
public:
  using Fn = std::function<void(Element &)>;

  explicit ReadMWMFunctor(Fn const & fn) : m_fn(fn) {}

  template <typename T>
  std::enable_if_t<std::is_same<T, FeatureID const>::value> ProcessElement(FeatureSource & src,
                                                                           uint32_t index) const
  {
    if (FeatureStatus::Deleted == src.GetFeatureStatus(index))
      return;

    m_fn(src.GetFeatureId(index));
  }

  template <typename T>
  std::enable_if_t<std::is_same<T, FeatureType>::value> ProcessElement(FeatureSource & src,
                                                                     uint32_t index) const
  {
    FeatureType feature;
    switch (src.GetFeatureStatus(index))
    {
    case FeatureStatus::Created: CHECK(false, ("Created features index should be generated."));
    case FeatureStatus::Deleted:
    case FeatureStatus::Obsolete: return;
    case FeatureStatus::Modified:
    {
      VERIFY(src.GetModifiedFeature(index, feature), ());
      break;
    }
    case FeatureStatus::Untouched:
    {
      src.GetOriginalFeature(index, feature);
      break;
    }
    }
    m_fn(feature);
  }

  // Reads features visible at |scale| covered by |cov| from mwm and applies |m_fn| to it.
  // Feature reading process consists of two steps: untouched (original) features reading and
  // touched (created, edited etc.) features reading.
  void operator()(MwmSet::MwmHandle const & handle, covering::CoveringGetter & cov, int scale) const
  {
    FeatureSource src(handle);

    MwmValue const * pValue = handle.GetValue<MwmValue>();
    if (pValue)
    {
      // Untouched (original) features reading. Applies covering |cov| to geometry index, gets
      // feature ids from it, gets untouched features by ids from |src| and applies |m_fn| by
      // ProcessElement.
      feature::DataHeader const & header = pValue->GetHeader();
      CheckUniqueIndexes checkUnique(header.GetFormat() >= version::Format::v5);

      // In case of WorldCoasts we should pass correct scale in ForEachInIntervalAndScale.
      auto const lastScale = header.GetLastScale();
      if (scale > lastScale)
        scale = lastScale;

      // Use last coding scale for covering (see index_builder.cpp).
      covering::Intervals const & intervals = cov.Get<RectId::DEPTH_LEVELS>(lastScale);
      ScaleIndex<ModelReaderPtr> index(pValue->m_cont.GetReader(INDEX_FILE_TAG), pValue->m_factory);

      // iterate through intervals
      for (auto const & i : intervals)
      {
        index.ForEachInIntervalAndScale(i.first, i.second, scale, [&](uint32_t index) {
          if (!checkUnique(index))
            return;
          ProcessElement<Element>(src, index);
        });
      }
    }
    // Check created features container.
    // Need to do it on a per-mwm basis, because Drape relies on features in a sorted order.
    // Touched (created, edited) features reading.
    src.ForEachInRectAndScale(cov.GetRect(), scale, m_fn);
  }

private:
  Fn m_fn;
};

class DataSourceBase : public MwmSet
{
public:
  using FeatureConstCallback = std::function<void(FeatureType const &)>;
  using FeatureCallback = std::function<void(FeatureType &)>;
  using FeatureIdCallback = std::function<void(FeatureID const &)>;

  ~DataSourceBase() override = default;

  /// Registers a new map.
  std::pair<MwmId, RegResult> RegisterMap(platform::LocalCountryFile const & localFile);

  /// Deregisters a map from internal records.
  ///
  /// \param countryFile A countryFile denoting a map to be deregistered.
  /// \return True if the map was successfully deregistered. If map is locked
  ///         now, returns false.
  bool DeregisterMap(platform::CountryFile const & countryFile);

  virtual void ForEachFeatureIDInRect(FeatureIdCallback const & f, m2::RectD const & rect,
                                      int scale) const = 0;
  virtual void ForEachInRect(FeatureCallback const & f, m2::RectD const & rect,
                             int scale) const = 0;
  virtual void ForEachInScale(FeatureCallback const & f, int scale) const = 0;
  virtual void ForEachInRectForMWM(FeatureCallback const & f, m2::RectD const & rect, int scale,
                                   MwmId const & id) const = 0;
  // "features" must be sorted using FeatureID::operator< as predicate.
  virtual void ReadFeatures(FeatureConstCallback const & fn,
                            std::vector<FeatureID> const & features) const = 0;

  void ReadFeature(FeatureConstCallback const & fn, FeatureID const & feature) const
  {
    return ReadFeatures(fn, {feature});
  }

protected:
  using ReaderCallback = std::function<void(MwmSet::MwmHandle const & handle,
                                            covering::CoveringGetter & cov, int scale)>;

  void ForEachInIntervals(ReaderCallback const & fn, covering::CoveringMode mode,
                          m2::RectD const & rect, int scale) const;

  /// MwmSet overrides:
  std::unique_ptr<MwmInfo> CreateInfo(platform::LocalCountryFile const & localFile) const override;
  std::unique_ptr<MwmValueBase> CreateValue(MwmInfo & info) const override;
};

template <typename FeatureSource>
class DataSourceImpl : public DataSourceBase
{
public:
  void ForEachFeatureIDInRect(FeatureIdCallback const & f, m2::RectD const & rect,
                              int scale) const override
  {
    ReadMWMFunctor<FeatureSource, FeatureID const> readFunctor(f);
    ForEachInIntervals(readFunctor, covering::LowLevelsOnly, rect, scale);
  }

  void ForEachInRect(FeatureCallback const & f, m2::RectD const & rect, int scale) const override
  {
    ReadMWMFunctor<FeatureSource, FeatureType> readFunctor(f);
    ForEachInIntervals(readFunctor, covering::ViewportWithLowLevels, rect, scale);
  }

  void ForEachInScale(FeatureCallback const & f, int scale) const override
  {
    ReadMWMFunctor<FeatureSource, FeatureType> readFunctor(f);
    ForEachInIntervals(readFunctor, covering::FullCover, m2::RectD::GetInfiniteRect(), scale);
  }

  void ForEachInRectForMWM(FeatureCallback const & f, m2::RectD const & rect, int scale,
                           MwmId const & id) const override
  {
    MwmHandle const handle = GetMwmHandleById(id);
    if (handle.IsAlive())
    {
      covering::CoveringGetter cov(rect, covering::ViewportWithLowLevels);
      ReadMWMFunctor<FeatureSource, FeatureType> readFunctor(f);
      readFunctor(handle, cov, scale);
    }
  }

  void ReadFeatures(FeatureConstCallback const & fn,
                    std::vector<FeatureID> const & features) const override
  {
    ASSERT(std::is_sorted(features.begin(), features.end()), ());

    auto fidIter = features.begin();
    auto const endIter = features.end();
    while (fidIter != endIter)
    {
      MwmId const & id = fidIter->m_mwmId;
      MwmHandle const handle = GetMwmHandleById(id);
      if (handle.IsAlive())
      {
        // Prepare features reading.
        FeatureSource src(handle);
        do
        {
          auto const fts = src.GetFeatureStatus(fidIter->m_index);
          ASSERT_NOT_EQUAL(
              FeatureStatus::Deleted, fts,
              ("Deleted feature was cached. It should not be here. Please review your code."));
          FeatureType featureType;
          if (fts == FeatureStatus::Modified || fts == FeatureStatus::Created)
            VERIFY(src.GetModifiedFeature(fidIter->m_index, featureType), ());
          else
            src.GetOriginalFeature(fidIter->m_index, featureType);
          fn(featureType);
        } while (++fidIter != endIter && id == fidIter->m_mwmId);
      }
      else
      {
        // Skip unregistered mwm files.
        while (++fidIter != endIter && id == fidIter->m_mwmId)
          ;
      }
    }
  }

  /// Guard for loading features from particular MWM by demand.
  /// @note This guard is suitable when mwm is loaded.
  class FeaturesLoaderGuard
  {
  public:
    FeaturesLoaderGuard(DataSourceBase const & index, MwmId const & id)
      : m_handle(index.GetMwmHandleById(id)), m_source(m_handle)
    {
    }

    MwmSet::MwmId const & GetId() const { return m_handle.GetId(); }

    std::string GetCountryFileName() const
    {
      if (!m_handle.IsAlive())
        return string();

      return m_handle.GetValue<MwmValue>()->GetCountryFileName();
    }

    bool IsWorld() const
    {
      if (!m_handle.IsAlive())
        return false;

      return m_handle.GetValue<MwmValue>()->GetHeader().GetType() == feature::DataHeader::world;
    }

    std::unique_ptr<FeatureType> GetOriginalFeatureByIndex(uint32_t index) const
    {
      auto feature = make_unique<FeatureType>();
      if (GetOriginalFeatureByIndex(index, *feature))
        return feature;

      return {};
    }

    std::unique_ptr<FeatureType> GetOriginalOrEditedFeatureByIndex(uint32_t index) const
    {
      auto feature = make_unique<FeatureType>();
      if (!m_handle.IsAlive())
        return {};

      ASSERT_NOT_EQUAL(m_source.GetFeatureStatus(index), FeatureStatus::Created, ());
      if (GetFeatureByIndex(index, *feature))
        return feature;

      return {};
    }

    /// Everyone, except Editor core, should use this method.
    WARN_UNUSED_RESULT bool GetFeatureByIndex(uint32_t index, FeatureType & ft) const
    {
      if (!m_handle.IsAlive())
        return false;

      ASSERT_NOT_EQUAL(
          FeatureStatus::Deleted, m_source.GetFeatureStatus(index),
          ("Deleted feature was cached. It should not be here. Please review your code."));
      if (m_source.GetModifiedFeature(index, ft))
        return true;
      return GetOriginalFeatureByIndex(index, ft);
    }

    /// Editor core only method, to get 'untouched', original version of feature.
    WARN_UNUSED_RESULT bool GetOriginalFeatureByIndex(uint32_t index, FeatureType & ft) const
    {
      return m_handle.IsAlive() ? m_source.GetOriginalFeature(index, ft) : false;
    }

    size_t GetNumFeatures() const { return m_source.GetNumFeatures(); }

  private:
    MwmHandle m_handle;
    FeatureSource m_source;
  };
};

using DataSource = DataSourceImpl<FeatureSource>;
