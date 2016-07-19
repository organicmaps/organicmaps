#pragma once
#include "indexer/data_header.hpp"
#include "indexer/feature_meta.hpp"

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

  IntervalIndexIFace * CreateIndex(ModelReaderPtr reader) const;
};
