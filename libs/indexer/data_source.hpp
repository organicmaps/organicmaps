#pragma once

#include "indexer/feature_covering.hpp"
#include "indexer/feature_source.hpp"
#include "indexer/mwm_set.hpp"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

class DataSource : public MwmSet
{
public:
  using FeatureCallback = std::function<void(FeatureType &)>;
  using FeatureIdCallback = std::function<void(FeatureID const &)>;
  using StopSearchCallback = std::function<bool(void)>;

  /// Registers a new map.
  std::pair<MwmId, RegResult> RegisterMap(platform::LocalCountryFile const & localFile);

  /// Deregisters a map from internal records.
  ///
  /// \param countryFile A countryFile denoting a map to be deregistered.
  /// \return True if the map was successfully deregistered. If map is locked
  ///         now, returns false.
  bool DeregisterMap(platform::CountryFile const & countryFile);

  void ForEachFeatureIDInRect(FeatureIdCallback const & f, m2::RectD const & rect, int scale,
                              covering::CoveringMode mode = covering::ViewportWithLowLevels) const;
  void ForEachInRect(FeatureCallback const & f, m2::RectD const & rect, int scale) const;
  // Calls |f| for features closest to |center| until |stopCallback| returns true or distance
  // |sizeM| from has been reached. Then for EditableDataSource calls |f| for each edited feature
  // inside square with center |center| and side |2 * sizeM|. Edited features are not in the same
  // hierarchy and there is no fast way to merge frozen and edited features.
  void ForClosestToPoint(FeatureCallback const & f, StopSearchCallback const & stopCallback, m2::PointD const & center,
                         double sizeM, int scale) const;
  void ForEachInScale(FeatureCallback const & f, int scale) const;
  void ForEachInRectForMWM(FeatureCallback const & f, m2::RectD const & rect, int scale, MwmId const & id) const;
  // "features" must be sorted using FeatureID::operator< as predicate.
  void ReadFeatures(FeatureCallback const & fn, std::vector<FeatureID> const & features) const;

  void ReadFeature(FeatureCallback const & fn, FeatureID const & feature) const { return ReadFeatures(fn, {feature}); }

  std::unique_ptr<FeatureSource> CreateFeatureSource(DataSource::MwmHandle const & handle) const
  {
    return (*m_factory)(handle);
  }

protected:
  using ReaderCallback =
      std::function<void(MwmSet::MwmHandle const & handle, covering::CoveringGetter & cov, int scale)>;

  explicit DataSource(std::unique_ptr<FeatureSourceFactory> factory) : m_factory(std::move(factory)) {}

  void ForEachInIntervals(ReaderCallback const & fn, covering::CoveringMode mode, m2::RectD const & rect,
                          int scale) const;

  /// @name MwmSet overrides
  /// @{
  std::unique_ptr<MwmInfo> CreateInfo(platform::LocalCountryFile const & localFile) const override;
  std::unique_ptr<MwmValue> CreateValue(MwmInfo & info) const override;
  /// @}

private:
  std::unique_ptr<FeatureSourceFactory> m_factory;
};

// DataSource which operates with features from mwm file and does not support features creation
// deletion or modification.
class FrozenDataSource : public DataSource
{
public:
  FrozenDataSource() : DataSource(std::make_unique<FeatureSourceFactory>()) {}
};

/// Guard for loading features from particular MWM by demand.
/// @note If you need to work with FeatureType from different threads you need to use
/// a unique FeaturesLoaderGuard instance for every thread.
/// For an example of concurrent extracting feature details please see ConcurrentFeatureParsingTest.
class FeaturesLoaderGuard
{
public:
  FeaturesLoaderGuard(DataSource const & dataSource, DataSource::MwmId const & id)
    : m_handle(dataSource.GetMwmHandleById(id))
    , m_source(dataSource.CreateFeatureSource(m_handle))
  {
    // FeaturesLoaderGuard is always created in-place, so MWM should always be alive.
    ASSERT(id.IsAlive(), ());
  }

  MwmSet::MwmId const & GetId() const { return m_handle.GetId(); }
  MwmSet::MwmHandle const & GetHandle() const { return m_handle; }

  std::string GetCountryFileName() const;
  int64_t GetVersion() const;

  bool IsWorld() const;
  /// Editor core only method, to get 'untouched', original version of feature.
  std::unique_ptr<FeatureType> GetOriginalFeatureByIndex(uint32_t index) const;
  std::unique_ptr<FeatureType> GetOriginalOrEditedFeatureByIndex(uint32_t index) const;
  /// Everyone, except Editor core, should use this method.
  std::unique_ptr<FeatureType> GetFeatureByIndex(uint32_t index) const;
  size_t GetNumFeatures() const { return m_source->GetNumFeatures(); }

private:
  MwmSet::MwmHandle m_handle;
  std::unique_ptr<FeatureSource> m_source;
};
