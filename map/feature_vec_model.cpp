#include "base/SRC_FIRST.hpp"

#include "map/feature_vec_model.hpp"

#include "platform/platform.hpp"

#include "indexer/cell_coverer.hpp"
#include "indexer/scales.hpp"
#include "indexer/classificator_loader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/bind.hpp"


namespace model
{

// While reading any files (classificator or mwm), there are 2 types of possible exceptions:
// Reader::Exception, FileAbsentException.
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

pair<MwmSet::MwmLock, bool> FeaturesFetcher::RegisterMap(string const & file)
{
  try
  {
    pair<MwmSet::MwmLock, bool> p = m_multiIndex.RegisterMap(file);
    if (!p.second)
    {
      LOG(LWARNING,
          ("Can't add map", file, "Probably it's already added or has newer data version."));
      return p;
    }
    MwmSet::MwmLock & lock = p.first;
    ASSERT(lock.IsLocked(), ("Mwm lock invariant violation."));
    m_rect.Add(lock.GetInfo()->m_limitRect);
    return p;
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("IO error while adding ", file, " map. ", e.what()));
    return make_pair(MwmSet::MwmLock(), false);
  }
}

void FeaturesFetcher::DeregisterMap(string const & file) { m_multiIndex.Deregister(file); }

void FeaturesFetcher::DeregisterAllMaps() { m_multiIndex.DeregisterAll(); }

bool FeaturesFetcher::DeleteMap(string const & file)
{
  return m_multiIndex.DeleteMap(file);
}

pair<MwmSet::MwmLock, Index::UpdateStatus> FeaturesFetcher::UpdateMap(string const & file)
{
  return m_multiIndex.UpdateMap(file);
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
