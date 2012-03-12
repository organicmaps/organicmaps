#include "../base/SRC_FIRST.hpp"

#include "feature_vec_model.hpp"

#include "../platform/platform.hpp"

#include "../indexer/cell_coverer.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/classificator_loader.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"


namespace model
{

void FeaturesFetcher::InitClassificator()
{
  try
  {
    classificator::Load();
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

int FeaturesFetcher::AddMap(string const & file)
{
  int version = -1;
  try
  {
    m2::RectD r;
    version = m_multiIndex.Add(file, r);
    m_rect.Add(r);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Data file adding error: ", e.what()));
  }
  return version;
}

void FeaturesFetcher::RemoveMap(string const & fName)
{
  m_multiIndex.Remove(fName);
}

void FeaturesFetcher::RemoveAllCountries()
{
  m_multiIndex.RemoveAllCountries();
}

//void FeaturesFetcher::Clean()
//{
//  m_rect.MakeEmpty();
//  // TODO: m_multiIndex.Clear(); - is it needed?
//}

void FeaturesFetcher::ClearCaches()
{
  m_multiIndex.ClearCache();
}

bool FeaturesFetcher::IsCountryLoaded(m2::PointD const & pt) const
{
  vector<MwmInfo> info;
  m_multiIndex.GetMwmInfo(info);

  for (size_t i = 0; i < info.size(); ++i)
    if (info[i].isValid() && info[i].isCountry() &&
        info[i].m_limitRect.IsPointInside(pt))
    {
      return true;
    }

  return false;
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
