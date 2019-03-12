#pragma once

#include "search/house_to_street_table.hpp"
#include "search/lazy_centers_table.hpp"

#include "editor/osm_editor.hpp"

#include "indexer/feature_covering.hpp"
#include "indexer/feature_source.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/scale_index.hpp"
#include "indexer/unique_index.hpp"

#include "base/macros.hpp"

#include <cstdint>
#include <memory>
#include <string>

class MwmValue;

namespace search
{
void CoverRect(m2::RectD const & rect, int scale, covering::Intervals & result);

/// @todo Move this class into "index" library and make it more generic.
/// Now it duplicates "DataSource" functionality.
class MwmContext
{
public:
  explicit MwmContext(MwmSet::MwmHandle handle);

  inline MwmSet::MwmId const & GetId() const { return m_handle.GetId(); }
  inline string const & GetName() const { return GetInfo()->GetCountryName(); }
  inline std::shared_ptr<MwmInfo> const & GetInfo() const { return GetId().GetInfo(); }

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
    covering::Intervals intervals;
    CoverRect(rect, scale, intervals);
    ForEachIndex(intervals, scale, forward<Fn>(fn));
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

  WARN_UNUSED_RESULT inline bool GetCenter(uint32_t index, m2::PointD & center)
  {
    return m_centers.Get(index, center);
  }

  MwmSet::MwmHandle m_handle;
  MwmValue & m_value;

private:
  FeatureStatus GetEditedStatus(uint32_t index) const
  {
    return osm::Editor::Instance().GetFeatureStatus(GetId(), index);
  }

  template <class Fn>
  void ForEachIndexImpl(covering::Intervals const & intervals, uint32_t scale, Fn && fn) const
  {
    CheckUniqueIndexes checkUnique(m_value.GetHeader().GetFormat() >= version::Format::v5);
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

  DISALLOW_COPY_AND_MOVE(MwmContext);
};
}  // namespace search
