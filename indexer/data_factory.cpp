#include "indexer/data_factory.hpp"

#include "coding/files_container.hpp"

#include "defines.hpp"

void IndexFactory::Load(FilesContainerR const & cont)
{
  m_version = version::MwmVersion::Read(cont);
  if (m_version.GetFormat() < version::Format::v11)
    MYTHROW(CorruptedMwmFile, (cont.GetFileName()));

  m_header.Load(cont);

  if (cont.IsExist(REGION_INFO_FILE_TAG))
  {
    ReaderSource<FilesContainerR::TReader> src(cont.GetReader(REGION_INFO_FILE_TAG));
    m_regionData.Deserialize(src);
  }
}
