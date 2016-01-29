#pragma once

#include "indexer/features_vector.hpp"
#include "indexer/index.hpp"
#include "indexer/scale_index.hpp"

#include "base/macros.hpp"

class MwmValue;

namespace search
{
namespace v2
{
struct MwmContext
{
  MwmContext(MwmSet::MwmHandle handle);

  MwmSet::MwmHandle m_handle;
  MwmValue & m_value;
  FeaturesVector m_vector;
  ScaleIndex<ModelReaderPtr> m_index;

  inline MwmSet::MwmId const & GetId() const { return m_handle.GetId(); }
  inline string const & GetName() const { return GetInfo()->GetCountryName(); }
  inline shared_ptr<MwmInfo> const & GetInfo() const { return GetId().GetInfo(); }

  template <class TFn> void ForEachFeature(m2::RectD const & rect, TFn && fn)
  {
    feature::DataHeader const & header = m_value.GetHeader();
    covering::CoveringGetter covering(rect, covering::ViewportWithLowLevels);

    uint32_t const scale = header.GetLastScale();
    covering::IntervalsT const & interval = covering.Get(scale);

    CheckUniqueIndexes checkUnique(header.GetFormat() >= version::Format::v5);
    for (auto const & i : interval)
    {
      m_index.ForEachInIntervalAndScale([&](uint32_t index)
                                        {
                                          if (checkUnique(index))
                                          {
                                            FeatureType ft;
                                            GetFeature(index, ft);
                                            fn(ft);
                                          }
                                        }, i.first, i.second, scale);
    }
  }

  inline void GetFeature(uint32_t index, FeatureType & ft) const
  {
    m_vector.GetByIndex(index, ft);
    ft.SetID(FeatureID(GetId(), index));
  }

  DISALLOW_COPY_AND_MOVE(MwmContext);
};

}  // namespace v2
}  // namespace search
