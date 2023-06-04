#pragma once

#include "search/house_to_street_table.hpp"
#include "search/lazy_centers_table.hpp"

#include "editor/editable_feature_source.hpp"

#include "indexer/feature_covering.hpp"
#include "indexer/feature_source.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/scale_index.hpp"
#include "indexer/unique_index.hpp"

#include "base/macros.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

class MwmValue;

namespace search
{
void CoverRect(m2::RectD const & rect, int scale, covering::Intervals & result);

/// @todo Move this class into "index" library and make it more generic.
/// Now it duplicates "DataSource" functionality.
class MwmContext
{
public:
  struct MwmType
  {
    bool IsFirstBatchMwm(bool inViewport) const
    {
      if (inViewport)
        return m_viewportIntersected;
      return m_viewportIntersected || m_containsUserPosition || m_containsMatchedCity;
    }

    bool m_viewportIntersected = false;
    bool m_containsUserPosition = false;
    bool m_containsMatchedCity = false;
    bool m_containsMatchedState = false;
  };

  explicit MwmContext(MwmSet::MwmHandle handle);
  MwmContext(MwmSet::MwmHandle handle, MwmType type);

  MwmSet::MwmId const & GetId() const { return m_handle.GetId(); }
  std::string const & GetName() const { return GetInfo()->GetCountryName(); }
  std::shared_ptr<MwmInfo> const & GetInfo() const { return GetId().GetInfo(); }
  MwmType const & GetType() const
  {
    CHECK(m_type, ());
    return *m_type;
  }

  template <typename Fn>
  void ForEachIndex(covering::Intervals const & intervals, uint32_t scale, Fn && fn) const
  {
    ForEachIndexImpl(intervals, scale, [&](uint32_t index)
                     {
                       // TODO: Optimize deleted checks by getting vector of deleted indexes from
                       // the Editor.
                       if (GetEditedStatus(index) != FeatureStatus::Deleted)
                         fn(index);
                     });
  }

  template <typename Fn>
  void ForEachIndex(m2::RectD const & rect, Fn && fn) const
  {
    uint32_t const scale = m_value.GetHeader().GetLastScale();
    ForEachIndex(rect, scale, std::forward<Fn>(fn));
  }

  template <typename Fn>
  void ForEachIndex(m2::RectD const & rect, uint32_t scale, Fn && fn) const
  {
    covering::Intervals intervals;
    CoverRect(rect, m_value.GetHeader().GetLastScale(), intervals);
    ForEachIndex(intervals, scale, std::forward<Fn>(fn));
  }

  template <typename Fn>
  void ForEachFeature(m2::RectD const & rect, Fn && fn) const
  {
    uint32_t const scale = m_value.GetHeader().GetLastScale();
    covering::Intervals intervals;
    CoverRect(rect, scale, intervals);

    ForEachIndexImpl(intervals, scale, [&](uint32_t index) {
      auto ft = GetFeature(index);
      if (ft)
        fn(*ft);
    });
  }

  // Returns false if feature was deleted by user.
  std::unique_ptr<FeatureType> GetFeature(uint32_t index) const;

  [[nodiscard]] inline bool GetCenter(uint32_t index, m2::PointD & center)
  {
    return m_centers.Get(index, center);
  }

  MwmSet::MwmHandle m_handle;
  MwmValue const & m_value;

private:
  FeatureStatus GetEditedStatus(uint32_t index) const
  {
    return m_editableSource.GetFeatureStatus(index);
  }

  template <class Fn>
  void ForEachIndexImpl(covering::Intervals const & intervals, uint32_t scale, Fn && fn) const
  {
    CheckUniqueIndexes checkUnique;
    for (auto const & i : intervals)
      m_index.ForEachInIntervalAndScale(i.first, i.second, scale,
          [&](uint64_t /* key */, uint32_t value)
          {
            if (checkUnique(value))
              fn(value);
          });
  }

  FeaturesVector m_vector;
  ScaleIndex<ModelReaderPtr> m_index;
  LazyCentersTable m_centers;
  EditableFeatureSource m_editableSource;
  std::optional<MwmType> m_type;

  DISALLOW_COPY_AND_MOVE(MwmContext);
};
}  // namespace search
