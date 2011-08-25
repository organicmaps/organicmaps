#include "../base/SRC_FIRST.hpp"

#include "data_factory.hpp"
#include "interval_index.hpp"

#include "../defines.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_container.hpp"

#include "../base/start_mem_debug.hpp"


void IndexFactory::Load(FilesContainerR const & cont)
{
  m_header.Load(cont.GetReader(HEADER_FILE_TAG));
}

IntervalIndexIFace * IndexFactory::CreateIndex(ModelReaderPtr reader)
{
  return new IntervalIndex<ModelReaderPtr>(reader);
}
