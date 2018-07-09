#include "map/feature_vec_model.hpp"

#include "platform/platform.hpp"

#include "indexer/cell_coverer.hpp"
#include "indexer/scales.hpp"
#include "indexer/classificator_loader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/bind.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;

namespace model
{
FeaturesFetcher::FeaturesFetcher()
{
  m_dataSource.AddObserver(*this);
}

FeaturesFetcher::~FeaturesFetcher()
{
  m_dataSource.RemoveObserver(*this);
}

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

pair<MwmSet::MwmId, MwmSet::RegResult> FeaturesFetcher::RegisterMap(
    LocalCountryFile const & localFile)
{
  try
  {
    auto result = m_dataSource.RegisterMap(localFile);
    if (result.second != MwmSet::RegResult::Success)
    {
      LOG(LWARNING, ("Can't add map", localFile.GetCountryName(), "(", result.second, ").",
                     "Probably it's already added or has newer data version."));
    }
    else
    {
      MwmSet::MwmId const & id = result.first;
      ASSERT(id.IsAlive(), ());
      m_rect.Add(id.GetInfo()->m_bordersRect);
    }

    return result;
  }
  catch (RootException const & ex)
  {
    LOG(LERROR, ("IO error while adding", localFile.GetCountryName(), "map.", ex.Msg()));
    return make_pair(MwmSet::MwmId(), MwmSet::RegResult::BadFile);
  }
}

bool FeaturesFetcher::DeregisterMap(CountryFile const & countryFile)
{
  return m_dataSource.Deregister(countryFile);
}

void FeaturesFetcher::Clear() { m_dataSource.Clear(); }

void FeaturesFetcher::ClearCaches()
{
  m_dataSource.ClearCache();
}

void FeaturesFetcher::OnMapUpdated(platform::LocalCountryFile const & newFile,
                                   platform::LocalCountryFile const & oldFile)
{
  if (m_onMapDeregistered)
    m_onMapDeregistered(oldFile);
}

void FeaturesFetcher::OnMapDeregistered(platform::LocalCountryFile const & localFile)
{
  if (m_onMapDeregistered)
    m_onMapDeregistered(localFile);
}

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
