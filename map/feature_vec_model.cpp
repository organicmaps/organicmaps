#include "../base/SRC_FIRST.hpp"

#include "feature_vec_model.hpp"

#include "../platform/platform.hpp"

#include "../indexer/cell_coverer.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/classificator_loader.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"

#include "../base/start_mem_debug.hpp"

namespace model
{

void FeaturesFetcher::InitClassificator()
{
  Platform & p = GetPlatform();

  try
  {
    classificator::Read(p.GetReader("drawing_rules.bin"),
                        p.GetReader("classificator.txt"),
                        p.GetReader("visibility.txt"),
                        p.GetReader("types.txt"));
  }
  catch (FileAbsentException const & e)
  {
      LOG(LERROR, ("Classificator not found: ", e.what()));
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Classificator read error: ", e.what()));
  }
}

void FeaturesFetcher::AddMap(string const & file)
{
  try
  {
    m_multiIndex.Add(file);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Data file adding error: ", e.what()));
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

void FeaturesFetcher::ClearCaches()
{
  m_multiIndex.ClearCaches();
}

m2::RectD FeaturesFetcher::GetWorldRect() const
{
  if (m_rect == m2::RectD())
  {
    // rect is empty when now countries are loaded
    // return max global rect
    return m2::RectD(MercatorBounds::minX,
                     MercatorBounds::minY,
                     MercatorBounds::maxX,
                     MercatorBounds::maxY);
  }
  return m_rect;
}

}
