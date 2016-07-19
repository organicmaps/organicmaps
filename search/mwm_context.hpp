#pragma once

#include "search/house_to_street_table.hpp"

#include "indexer/features_vector.hpp"
#include "indexer/index.hpp"
#include "indexer/scale_index.hpp"

#include "base/macros.hpp"

#include "std/unique_ptr.hpp"

class MwmValue;

namespace search
{
void CoverRect(m2::RectD const & rect, int scale, covering::IntervalsT & result);

/// @todo Move this class into "index" library and make it more generic.
/// Now it duplicates "Index" functionality.
class MwmContext
{
public:
  MwmSet::MwmHandle m_handle;
  MwmValue & m_value;

private:
  FeaturesVector m_vector;
  ScaleIndex<ModelReaderPtr> m_index;
  unique_ptr<HouseToStreetTable> m_houseToStreetTable;

public:
  explicit MwmContext(MwmSet::MwmHandle handle);

  inline bool IsAlive() const { return m_handle.IsAlive(); }
  inline MwmSet::MwmId const & GetId() const { return m_handle.GetId(); }
  inline string const & GetName() const { return GetInfo()->GetCountryName(); }
  inline shared_ptr<MwmInfo> const & GetInfo() const { return GetId().GetInfo(); }

  template <typename TFn>
  void ForEachIndex(covering::IntervalsT const & intervals, uint32_t scale, TFn && fn) const
  {
    ForEachIndexImpl(intervals, scale, [&](uint32_t index)
                     {
                       // TODO: Optimize deleted checks by getting vector of deleted indexes from
                       // the Editor.
                       if (GetEditedStatus(index) != osm::Editor::FeatureStatus::Deleted)
                         fn(index);
                     });
  }

  template <typename TFn>
  void ForEachIndex(m2::RectD const & rect, TFn && fn) const
  {
    uint32_t const scale = m_value.GetHeader().GetLastScale();
    covering::IntervalsT intervals;
    CoverRect(rect, scale, intervals);
    ForEachIndex(intervals, scale, forward<TFn>(fn));
  }

  template <typename TFn>
  void ForEachFeature(m2::RectD const & rect, TFn && fn) const
  {
    uint32_t const scale = m_value.GetHeader().GetLastScale();
    covering::IntervalsT intervals;
    CoverRect(rect, scale, intervals);

    ForEachIndexImpl(intervals, scale, [&](uint32_t index)
                     {
                       FeatureType ft;
                       if (GetFeature(index, ft))
                         fn(ft);
                     });
  }

  // @returns false if feature was deleted by user.
  bool GetFeature(uint32_t index, FeatureType & ft) const;

  bool GetStreetIndex(uint32_t houseId, uint32_t & streetId);

private:
  osm::Editor::FeatureStatus GetEditedStatus(uint32_t index) const
  {
    return osm::Editor::Instance().GetFeatureStatus(GetId(), index);
  }

  template <class TFn>
  void ForEachIndexImpl(covering::IntervalsT const & intervals, uint32_t scale, TFn && fn) const
  {
    CheckUniqueIndexes checkUnique(m_value.GetHeader().GetFormat() >= version::Format::v5);
    for (auto const & i : intervals)
      m_index.ForEachInIntervalAndScale(
          [&](uint32_t index)
          {
            if (checkUnique(index))
              fn(index);
          },
          i.first, i.second, scale);
  }

  DISALLOW_COPY_AND_MOVE(MwmContext);
};
}  // namespace search
