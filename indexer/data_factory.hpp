#pragma once
#include "indexer/data_header.hpp"

#include "platform/mwm_version.hpp"


class FilesContainerR;
class IntervalIndexIFace;

class IndexFactory
{
  version::MwmVersion m_version;
  feature::DataHeader m_header;

public:
  void Load(FilesContainerR const & cont);

  inline version::MwmVersion const & GetMwmVersion() const { return m_version; }
  inline feature::DataHeader const & GetHeader() const { return m_header; }

  IntervalIndexIFace * CreateIndex(ModelReaderPtr reader) const;
};
