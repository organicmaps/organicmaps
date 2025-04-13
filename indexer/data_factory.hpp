#pragma once
#include "indexer/data_header.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/interval_index.hpp"

#include "platform/mwm_version.hpp"

#include <memory>

class FilesContainerR;

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
  inline void MoveRegionData(feature::RegionData & data) { data = std::move(m_regionData); }

  template <typename Reader>
  std::unique_ptr<IntervalIndex<Reader, uint32_t>> CreateIndex(Reader const & reader) const
  {
    return std::make_unique<IntervalIndex<Reader, uint32_t>>(reader);
  }
};
