#include "../base/SRC_FIRST.hpp"

#include "feature_vec_model.hpp"

#include "../platform/platform.hpp"

#include "../indexer/cell_coverer.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/classif_routine.hpp"
#include "../indexer/classificator.hpp"

#include "../coding/file_container.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"

#include "../base/start_mem_debug.hpp"

namespace model
{

void FeaturesFetcher::InitClassificator()
{
  Platform & p = GetPlatform();
  classificator::Read(p.ReadPathForFile("drawing_rules.bin"),
                      p.ReadPathForFile("classificator.txt"),
                      p.ReadPathForFile("visibility.txt"));
}

void FeaturesFetcher::AddMap(string const & fName)
{
  if (m_multiIndex.IsExist(fName))
    return;

  try
  {
    uint32_t const logPageSize = 12;
    uint32_t const logPageCount = 12;

    FilesContainerR container(fName, logPageSize, logPageCount);
    m_multiIndex.Add(FeatureReaders<FileReader>(container), container.GetReader(INDEX_FILE_TAG));
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

void FeaturesFetcher::RemoveMap(string const & fName)
{
  m_multiIndex.Remove(fName);
}

void FeaturesFetcher::Clean()
{
  m_rect.MakeEmpty();
  m_multiIndex.Clean();
}

}
