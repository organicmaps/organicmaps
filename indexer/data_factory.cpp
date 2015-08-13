#include "indexer/data_factory.hpp"
#include "indexer/interval_index.hpp"
#include "indexer/old/interval_index_101.hpp"


void IndexFactory::Load(FilesContainerR const & cont)
{
  ReadVersion(cont, m_version);
  m_header.Load(cont);
}

IntervalIndexIFace * IndexFactory::CreateIndex(ModelReaderPtr reader) const
{
  if (m_version.format == version::v1)
    return new old_101::IntervalIndex<uint32_t, ModelReaderPtr>(reader);
  return new IntervalIndex<ModelReaderPtr>(reader);
}
