#include "base/SRC_FIRST.hpp"

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
  m_multiIndex.AddObserver(*this);
}

FeaturesFetcher::~FeaturesFetcher()
{
  m_multiIndex.RemoveObserver(*this);
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

pair<MwmSet::MwmHandle, MwmSet::RegResult> FeaturesFetcher::RegisterMap(
    LocalCountryFile const & localFile)
{
  string const countryFileName = localFile.GetCountryName();
  try
  {
    auto result = m_multiIndex.RegisterMap(localFile);
    if (result.second != MwmSet::RegResult::Success)
    {
      LOG(LWARNING, ("Can't add map", countryFileName,
                     "Probably it's already added or has newer data version."));
      return result;
    }
    MwmSet::MwmHandle & handle = result.first;
    ASSERT(handle.IsAlive(), ("Mwm lock invariant violation."));
    m_rect.Add(handle.GetInfo()->m_limitRect);
    return result;
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("IO error while adding ", countryFileName, " map. ", e.what()));
    return make_pair(MwmSet::MwmHandle(), MwmSet::RegResult::BadFile);
  }
}

bool FeaturesFetcher::DeregisterMap(CountryFile const & countryFile)
{
  return m_multiIndex.Deregister(countryFile);
}

void FeaturesFetcher::Clear() { m_multiIndex.Clear(); }

void FeaturesFetcher::ClearCaches()
{
  m_multiIndex.ClearCache();
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
