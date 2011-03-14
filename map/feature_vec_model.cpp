#include "../base/SRC_FIRST.hpp"

#include "feature_vec_model.hpp"

#include "../platform/platform.hpp"

#include "../indexer/cell_coverer.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/classif_routine.hpp"
#include "../indexer/classificator.hpp"

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
  try
  {
    m_multiIndex.Add(fName);
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
