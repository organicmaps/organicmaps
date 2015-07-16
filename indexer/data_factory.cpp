#include "indexer/data_factory.hpp"
#include "indexer/interval_index.hpp"
#include "indexer/old/interval_index_101.hpp"
#include "indexer/mwm_version.hpp"

#include "defines.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_container.hpp"


using FHeaderT = feature::DataHeader;

void LoadMapHeader(FilesContainerR const & cont, FHeaderT & header)
{
  ModelReaderPtr headerReader = cont.GetReader(HEADER_FILE_TAG);
  version::MwmVersion version;

  if (version::ReadVersion(cont, version))
    header.Load(headerReader, version.format);
  else
    header.LoadV1(headerReader);
}

void LoadMapHeader(ModelReaderPtr const & reader, FHeaderT & header)
{
  LoadMapHeader(FilesContainerR(reader), header);
}

void IndexFactory::Load(FilesContainerR const & cont)
{
  ReadVersion(cont, m_version);
  LoadMapHeader(cont, m_header);
}

IntervalIndexIFace * IndexFactory::CreateIndex(ModelReaderPtr reader)
{
  if (m_version.format == version::v1)
    return new old_101::IntervalIndex<uint32_t, ModelReaderPtr>(reader);
  return new IntervalIndex<ModelReaderPtr>(reader);
}
