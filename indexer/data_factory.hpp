#pragma once
#include "indexer/data_header.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/interval_index.hpp"
#include "indexer/old/interval_index_101.hpp"

#include "platform/mwm_version.hpp"

class FilesContainerR;
class IntervalIndexIFace;

class IndexFactory
{
  version::MwmVersion m_version;
  feature::DataHeader m_header;
  feature::RegionData m_regionData;

public:
  void Load(FilesContainerR const & cont);

  inline version::MwmVersion const & GetMwmVersion() const { return m_version; }
  inline feature::DataHeader const & GetHeader() const { return m_header; }
  inline feature::RegionData const & GetRegionData() const { return m_regionData; }

  template <typename Reader>
  IntervalIndexIFace * CreateIndex(Reader const & reader) const
  {
    if (m_version.GetFormat() == version::Format::v1)
      return new old_101::IntervalIndex<uint32_t, Reader>(reader);
    return new IntervalIndex<Reader>(reader);
  }
};
