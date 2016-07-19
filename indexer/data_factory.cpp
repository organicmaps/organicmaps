#include "indexer/data_factory.hpp"
#include "indexer/interval_index.hpp"
#include "indexer/old/interval_index_101.hpp"

#include "coding/file_container.hpp"

#include "defines.hpp"


void IndexFactory::Load(FilesContainerR const & cont)
{
  ReadVersion(cont, m_version);
  m_header.Load(cont);

  if (cont.IsExist(REGION_INFO_FILE_TAG))
  {
    ReaderSource<FilesContainerR::TReader> src(cont.GetReader(REGION_INFO_FILE_TAG));
    m_regionData.Deserialize(src);
  }
}

IntervalIndexIFace * IndexFactory::CreateIndex(ModelReaderPtr reader) const
{
  if (m_version.GetFormat() == version::Format::v1)
    return new old_101::IntervalIndex<uint32_t, ModelReaderPtr>(reader);
  return new IntervalIndex<ModelReaderPtr>(reader);
}
