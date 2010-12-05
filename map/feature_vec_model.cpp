#include "../base/SRC_FIRST.hpp"

#include "feature_vec_model.hpp"

#include "../platform/platform.hpp"

#include "../indexer/cell_coverer.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/classif_routine.hpp"
#include "../indexer/classificator.hpp"
#include "../indexer/feature_processor.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"

#include "../base/start_mem_debug.hpp"

namespace model
{

void FeaturesFetcher::InitClassificator()
{
  classificator::Read(GetPlatform().ResourcesDir());
}

void FeaturesFetcher::AddMap(string const & dataPath, string const & indexPath)
{
  if (m_multiIndex.IsExist(dataPath))
    return;

  try
  {
    uint32_t const datLogPageSize = 10;
    uint32_t const datLogPageCount = 11;

    uint32_t const idxLogPageSize = 8;
    uint32_t const idxLogPageCount = 11;

    FileReader dataReader(dataPath, datLogPageSize, datLogPageCount);
    uint64_t const startOffset = feature::ReadDatHeaderSize(dataReader);

#ifdef USE_BUFFER_READER
    // readers from memory
    m_multiIndex.Add(BufferReader(dataReader, startOffset), BufferReader(FileReader(indexPath)));
#else
    // readers from file
    m_multiIndex.Add(
        dataReader.SubReader(startOffset, dataReader.Size() - startOffset),
        FileReader(indexPath, idxLogPageSize, idxLogPageCount));
#endif
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("Data file not found: ", e.what()));
  }
  catch (Reader::Exception const & e)
  {
    LOG(LCRITICAL, ("Unknown error while reading file: ", e.what()));
  }
}

void FeaturesFetcher::RemoveMap(string const & dataPath)
{
  m_multiIndex.Remove(dataPath);
}

void FeaturesFetcher::Clean()
{
  m_rect.MakeEmpty();
  m_multiIndex.Clean();
}

}
