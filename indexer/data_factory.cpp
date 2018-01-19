#include "indexer/data_factory.hpp"

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
