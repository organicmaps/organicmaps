#include "../base/SRC_FIRST.hpp"

#include "data_factory.hpp"
#include "interval_index.hpp"
#include "old/interval_index_101.hpp"
#include "mwm_version.hpp"

#include "../defines.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_container.hpp"


typedef feature::DataHeader FHeaderT;

void LoadMapHeader(FilesContainerR const & cont, FHeaderT & header)
{
  ModelReaderPtr headerReader = cont.GetReader(HEADER_FILE_TAG);

  if (!cont.IsExist(VERSION_FILE_TAG))
    header.LoadVer1(headerReader);
  else
  {
    ModelReaderPtr verReader = cont.GetReader(VERSION_FILE_TAG);
    header.Load(headerReader, static_cast<FHeaderT::Version>(ver::ReadVersion(verReader)));
  }
}

void LoadMapHeader(ModelReaderPtr const & reader, FHeaderT & header)
{
  LoadMapHeader(FilesContainerR(reader), header);
}

void IndexFactory::Load(FilesContainerR const & cont)
{
  LoadMapHeader(cont, m_header);
}

IntervalIndexIFace * IndexFactory::CreateIndex(ModelReaderPtr reader)
{
  IntervalIndexIFace * p;

  switch (m_header.GetVersion())
  {
  case FHeaderT::v1:
    p = new old_101::IntervalIndex<uint32_t, ModelReaderPtr>(reader);
    break;

  default:
    p = new IntervalIndex<ModelReaderPtr>(reader);
    break;
  }

  return p;
}
