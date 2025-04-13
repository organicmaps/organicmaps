#include "map/features_fetcher.hpp"

#include "platform/platform.hpp"

#include "indexer/classificator_loader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;

FeaturesFetcher::FeaturesFetcher() { m_dataSource.AddObserver(*this); }

FeaturesFetcher::~FeaturesFetcher() { m_dataSource.RemoveObserver(*this); }

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

std::pair<MwmSet::MwmId, MwmSet::RegResult> FeaturesFetcher::RegisterMap(
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
    return std::make_pair(MwmSet::MwmId(), MwmSet::RegResult::BadFile);
  }
}

bool FeaturesFetcher::DeregisterMap(CountryFile const & countryFile)
{
  return m_dataSource.Deregister(countryFile);
}

void FeaturesFetcher::Clear() { m_dataSource.Clear(); }

void FeaturesFetcher::ClearCaches() { m_dataSource.ClearCache(); }

m2::RectD FeaturesFetcher::GetWorldRect() const
{
  if (m_rect == m2::RectD())
  {
    // Rect is empty when no countries are loaded, return max global rect.
    return mercator::Bounds::FullRect();
  }
  return m_rect;
}

void FeaturesFetcher::OnMapDeregistered(platform::LocalCountryFile const & localFile)
{
  if (m_onMapDeregistered)
    m_onMapDeregistered(localFile);
}
