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

// While reading any files (classificator or mwm), there are 2 types of possible exceptions:
// Reader::Exception; FileAbsentException; SourceOutOfBoundsException.
// Let's process RootException everywhere, to supress such errors.

void FeaturesFetcher::InitClassificator()
{
  try
  {
    classificator::Load();
  }
  catch (RootException const & e)
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

    if (version != -1)
      m_rect.Add(r);
    else
      LOG(LWARNING, ("Can't add map", file, "Probably it's already added or has newer data version."));
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("IO error while adding ", file, " map. ", e.what()));
  }

  return version;
}

void FeaturesFetcher::RemoveMap(string const & file)
{
  m_multiIndex.Remove(file);
}

bool FeaturesFetcher::DeleteMap(string const & file)
{
  return m_multiIndex.DeleteMap(file);
}

bool FeaturesFetcher::UpdateMap(string const & file, m2::RectD & rect)
{
  return m_multiIndex.UpdateMap(file, rect);
}

void FeaturesFetcher::RemoveAll()
{
  m_multiIndex.RemoveAll();
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

/*
bool FeaturesFetcher::IsLoaded(m2::PointD const & pt) const
{
  vector<MwmInfo> info;
  m_multiIndex.GetMwmInfo(info);

  for (size_t i = 0; i < info.size(); ++i)
    if (info[i].IsExist() &&
        info[i].GetType() == MwmInfo::COUNTRY &&
        info[i].m_limitRect.IsPointInside(pt))
    {
      return true;
    }

  return false;
}
*/

m2::RectD FeaturesFetcher::GetWorldRect() const
{
  if (m_rect == m2::RectD())
  {
    // rect is empty when now countries are loaded
    // return max global rect
    return MercatorBounds::FullRect();
  }
  return m_rect;
}

}
