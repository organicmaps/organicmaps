#include "../base/SRC_FIRST.hpp"

#include "data_factory.hpp"
#include "interval_index.hpp"
#include "old/interval_index_101.hpp"

#include "../defines.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_container.hpp"


void LoadMapHeader(FilesContainerR const & cont, feature::DataHeader & header)
{
  ModelReaderPtr r = cont.GetReader(HEADER_FILE_TAG);

  if (cont.IsReaderExist(VERSION_FILE_TAG))
    header.Load(r);
  else
    header.LoadVer1(r);
}

void LoadMapHeader(ModelReaderPtr const & reader, feature::DataHeader & header)
{
  LoadMapHeader(FilesContainerR(reader), header);
}

void IndexFactory::Load(FilesContainerR const & cont)
{
  LoadMapHeader(cont, m_header);
}

IntervalIndexIFace * IndexFactory::CreateIndex(ModelReaderPtr reader)
{
  using namespace feature;

  IntervalIndexIFace * p;

  switch (m_header.GetVersion())
  {
  case DataHeader::v1:
    p = new old_101::IntervalIndex<uint32_t, ModelReaderPtr>(reader);
    break;

  default:
    p = new IntervalIndex<ModelReaderPtr>(reader);;
    break;
  }

  return p;
}
