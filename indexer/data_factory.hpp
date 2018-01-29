#pragma once
#include "indexer/data_header.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/interval_index.hpp"

#include "platform/mwm_version.hpp"

#include <memory>

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
  std::unique_ptr<IntervalIndex<Reader>> CreateIndex(Reader const & reader) const
  {
    CHECK_NOT_EQUAL(m_version.GetFormat(), version::Format::v1, ("Old maps format is not supported"));
    return std::make_unique<IntervalIndex<Reader>>(reader);
  }
};
