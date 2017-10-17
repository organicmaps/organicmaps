#pragma once
#include "indexer/cell_id.hpp"
#include "indexer/data_factory.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/osm_editor.hpp"
#include "indexer/scale_index.hpp"
#include "indexer/unique_index.hpp"

#include "coding/file_container.hpp"

#include "defines.hpp"

#include "base/macros.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

class MwmInfoEx : public MwmInfo
{
private:
  friend class Index;
  friend class MwmValue;

  // weak_ptr is needed here to access offsets table in already
  // instantiated MwmValue-s for the MWM, including MwmValues in the
  // MwmSet's cache. We can't use shared_ptr because of offsets table
  // must be removed as soon as the last corresponding MwmValue is
  // destroyed. Also, note that this value must be used and modified
  // only in MwmValue::SetTable() method, which, in turn, is called
  // only in the MwmSet critical section, protected by a lock.  So,
  // there's an implicit synchronization on this field.
  std::weak_ptr<feature::FeaturesOffsetsTable> m_table;
};

class MwmValue : public MwmSet::MwmValueBase
{
public:
  FilesContainerR const m_cont;
  IndexFactory m_factory;
  platform::LocalCountryFile const m_file;

  std::shared_ptr<feature::FeaturesOffsetsTable> m_table;

  explicit MwmValue(platform::LocalCountryFile const & localFile);
  void SetTable(MwmInfoEx & info);

  inline feature::DataHeader const & GetHeader() const { return m_factory.GetHeader(); }
  inline feature::RegionData const & GetRegionData() const { return m_factory.GetRegionData(); }
  inline version::MwmVersion const & GetMwmVersion() const { return m_factory.GetMwmVersion(); }
  inline std::string const & GetCountryFileName() const
  {
    return m_file.GetCountryFile().GetName();
  }

  inline bool HasSearchIndex() { return m_cont.IsExist(SEARCH_INDEX_FILE_TAG); }
  inline bool HasGeometryIndex() { return m_cont.IsExist(INDEX_FILE_TAG); }
};

class Index : public MwmSet
{
protected:
  /// MwmSet overrides:
  //@{
  std::unique_ptr<MwmInfo> CreateInfo(platform::LocalCountryFile const & localFile) const override;

  std::unique_ptr<MwmValueBase> CreateValue(MwmInfo & info) const override;
  //@}

public:
  /// Registers a new map.
  std::pair<MwmId, RegResult> RegisterMap(platform::LocalCountryFile const & localFile);

  /// Deregisters a map from internal records.
  ///
  /// \param countryFile A countryFile denoting a map to be deregistered.
  /// \return True if the map was successfully deregistered. If map is locked
  ///         now, returns false.
  bool DeregisterMap(platform::CountryFile const & countryFile);

private:

  template <typename F> class ReadMWMFunctor
  {
    F & m_f;
    osm::Editor & m_editor = osm::Editor::Instance();
  public:
    ReadMWMFunctor(F & f) : m_f(f) {}

    /// Used by Editor to inject new features.
    void operator()(FeatureType & feature)
    {
      m_f(feature);
    }

    void operator()(MwmHandle const & handle, covering::CoveringGetter & cov, int scale) const
    {
      MwmValue const * pValue = handle.GetValue<MwmValue>();
      if (pValue)
      {
        feature::DataHeader const & header = pValue->GetHeader();

        // Prepare needed covering.
        auto const lastScale = header.GetLastScale();

        // In case of WorldCoasts we should pass correct scale in ForEachInIntervalAndScale.
        if (scale > lastScale)
          scale = lastScale;

        // Use last coding scale for covering (see index_builder.cpp).
        covering::IntervalsT const & interval = cov.Get(lastScale);

        // Prepare features reading.
        FeaturesVector const fv(pValue->m_cont, header, pValue->m_table.get());
        ScaleIndex<ModelReaderPtr> index(pValue->m_cont.GetReader(INDEX_FILE_TAG),
                                         pValue->m_factory);

        // iterate through intervals
        CheckUniqueIndexes checkUnique(header.GetFormat() >= version::Format::v5);
        MwmId const & mwmID = handle.GetId();

        for (auto const & i : interval)
        {
          index.ForEachInIntervalAndScale(
              [&](uint32_t index)
              {
                if (!checkUnique(index))
                  return;

                FeatureType feature;
                switch (m_editor.GetFeatureStatus(mwmID, index))
                {
                case osm::Editor::FeatureStatus::Deleted:
                case osm::Editor::FeatureStatus::Obsolete:
                  return;
                case osm::Editor::FeatureStatus::Modified:
                  VERIFY(m_editor.GetEditedFeature(mwmID, index, feature), ());
                  m_f(feature);
                  return;
                case osm::Editor::FeatureStatus::Created:
                  CHECK(false, ("Created features index should be generated."));
                case osm::Editor::FeatureStatus::Untouched: break;
                }

                fv.GetByIndex(index, feature);
                feature.SetID(FeatureID(mwmID, index));
                m_f(feature);
              },
              i.first, i.second, scale);
        }
      }
    }
  };

  template <typename F> class ReadFeatureIndexFunctor
  {
    F & m_f;
    osm::Editor & m_editor = osm::Editor::Instance();
  public:
    ReadFeatureIndexFunctor(F & f) : m_f(f) {}

    /// Used by Editor to inject new features.
    void operator()(FeatureID const & fid) const
    {
      m_f(fid);
    }

    void operator()(MwmHandle const & handle, covering::CoveringGetter & cov, int scale) const
    {
      MwmValue const * pValue = handle.GetValue<MwmValue>();
      if (pValue)
      {
        feature::DataHeader const & header = pValue->GetHeader();

        // Prepare needed covering.
        int const lastScale = header.GetLastScale();

        // In case of WorldCoasts we should pass correct scale in ForEachInIntervalAndScale.
        if (scale > lastScale)
          scale = lastScale;

        // Use last coding scale for covering (see index_builder.cpp).
        covering::IntervalsT const & interval = cov.Get(lastScale);
        ScaleIndex<ModelReaderPtr> const index(pValue->m_cont.GetReader(INDEX_FILE_TAG), pValue->m_factory);

        // Iterate through intervals.
        CheckUniqueIndexes checkUnique(header.GetFormat() >= version::Format::v5);
        MwmId const & mwmID = handle.GetId();

        for (auto const & i : interval)
        {
          index.ForEachInIntervalAndScale(
              [&](uint32_t index)
              {
                if (osm::Editor::FeatureStatus::Deleted !=
                        m_editor.GetFeatureStatus(mwmID, index) &&
                    checkUnique(index))
                  m_f(FeatureID(mwmID, index));
              },
              i.first, i.second, scale);
        }
      }
    }
  };

public:

  template <typename F>
  void ForEachInRect(F && f, m2::RectD const & rect, int scale) const
  {
    ReadMWMFunctor<F> implFunctor(f);
    ForEachInIntervals(implFunctor, covering::ViewportWithLowLevels, rect, scale);
  }

  template <typename F>
  void ForEachFeatureIDInRect(F && f, m2::RectD const & rect, int scale) const
  {
    ReadFeatureIndexFunctor<F> implFunctor(f);
    ForEachInIntervals(implFunctor, covering::LowLevelsOnly, rect, scale);
  }

  template <typename F>
  void ForEachInScale(F && f, int scale) const
  {
    ReadMWMFunctor<F> implFunctor(f);
    ForEachInIntervals(implFunctor, covering::FullCover, m2::RectD::GetInfiniteRect(), scale);
  }

  template <typename F>
  void ReadFeature(F && f, FeatureID const & feature) const
  {
    return ReadFeatures(forward<F>(f), {feature});
  }

  // "features" must be sorted using FeatureID::operator< as predicate.
  template <typename F>
  void ReadFeatures(F && f, std::vector<FeatureID> const & features) const
  {
    auto fidIter = features.begin();
    auto const endIter = features.end();
    auto & editor = osm::Editor::Instance();
    while (fidIter != endIter)
    {
      MwmId const & id = fidIter->m_mwmId;
      MwmHandle const handle = GetMwmHandleById(id);
      if (handle.IsAlive())
      {
        MwmValue const * pValue = handle.GetValue<MwmValue>();
        FeaturesVector const featureReader(pValue->m_cont, pValue->GetHeader(),
                                           pValue->m_table.get());
        do
        {
          osm::Editor::FeatureStatus const fts = editor.GetFeatureStatus(id, fidIter->m_index);
          ASSERT_NOT_EQUAL(osm::Editor::FeatureStatus::Deleted, fts,
                           ("Deleted feature was cached. It should not be here. Please review your code."));
          FeatureType featureType;
          if (fts == osm::Editor::FeatureStatus::Modified || fts == osm::Editor::FeatureStatus::Created)
          {
            VERIFY(editor.GetEditedFeature(id, fidIter->m_index, featureType), ());
          }
          else
          {
            featureReader.GetByIndex(fidIter->m_index, featureType);
            featureType.SetID(*fidIter);
          }
          f(featureType);
        }
        while (++fidIter != endIter && id == fidIter->m_mwmId);
      }
      else
      {
        // Skip unregistered mwm files.
        while (++fidIter != endIter && id == fidIter->m_mwmId);
      }
    }
  }

  /// Guard for loading features from particular MWM by demand.
  /// @note This guard is suitable when mwm is loaded.
  class FeaturesLoaderGuard
  {
  public:
    FeaturesLoaderGuard(Index const & index, MwmId const & id);

    inline MwmSet::MwmId const & GetId() const { return m_handle.GetId(); }
    std::string GetCountryFileName() const;
    bool IsWorld() const;

    std::unique_ptr<FeatureType> GetOriginalFeatureByIndex(uint32_t index) const;
    std::unique_ptr<FeatureType> GetOriginalOrEditedFeatureByIndex(uint32_t index) const;

    /// Everyone, except Editor core, should use this method.
    WARN_UNUSED_RESULT bool GetFeatureByIndex(uint32_t index, FeatureType & ft) const;

    /// Editor core only method, to get 'untouched', original version of feature.
    WARN_UNUSED_RESULT bool GetOriginalFeatureByIndex(uint32_t index, FeatureType & ft) const;

    size_t GetNumFeatures() const;


  private:
    MwmHandle m_handle;
    std::unique_ptr<FeaturesVector> m_vector;
    osm::Editor & m_editor = osm::Editor::Instance();
  };

  template <typename F>
  void ForEachInRectForMWM(F && f, m2::RectD const & rect, int scale, MwmId const & id) const
  {
    MwmHandle const handle = GetMwmHandleById(id);
    if (handle.IsAlive())
    {
      covering::CoveringGetter cov(rect, covering::ViewportWithLowLevels);
      ReadMWMFunctor<F> fn(f);
      fn(handle, cov, scale);
    }
  }

private:

  template <typename F>
  void ForEachInIntervals(F && f, covering::CoveringMode mode, m2::RectD const & rect,
                          int scale) const
  {
    std::vector<std::shared_ptr<MwmInfo>> mwms;
    GetMwmsInfo(mwms);

    covering::CoveringGetter cov(rect, mode);

    MwmId worldID[2];

    osm::Editor & editor = osm::Editor::Instance();

    for (shared_ptr<MwmInfo> const & info : mwms)
    {
      if (info->m_minScale <= scale && scale <= info->m_maxScale &&
          rect.IsIntersect(info->m_limitRect))
      {
        MwmId const id(info);
        switch (info->GetType())
        {
          case MwmInfo::COUNTRY:
          {
            MwmHandle const handle = GetMwmHandleById(id);
            f(handle, cov, scale);
            // Check created features container.
            // Need to do it on a per-mwm basis, because Drape relies on features in a sorted order.
            editor.ForEachFeatureInMwmRectAndScale(id, f, rect, scale);
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
      MwmHandle const handle = GetMwmHandleById(worldID[0]);
      f(handle, cov, scale);
      // Check edited/created features container.
      // Need to do it on a per-mwm basis, because Drape relies on features in a sorted order.
      editor.ForEachFeatureInMwmRectAndScale(worldID[0], f, rect, scale);
    }

    if (worldID[1].IsAlive())
    {
      MwmHandle const handle = GetMwmHandleById(worldID[1]);
      f(handle, cov, scale);
      // Check edited/created features container.
      // Need to do it on a per-mwm basis, because Drape relies on features in a sorted order.
      editor.ForEachFeatureInMwmRectAndScale(worldID[1], f, rect, scale);
    }
  }
};
