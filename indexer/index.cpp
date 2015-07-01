#include "indexer/index.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;

//////////////////////////////////////////////////////////////////////////////////
// MwmValue implementation
//////////////////////////////////////////////////////////////////////////////////

MwmValue::MwmValue(LocalCountryFile const & localFile)
    : m_cont(GetPlatform().GetCountryReader(localFile, TMapOptions::EMap)),
      m_countryFile(localFile.GetCountryFile())
{
  m_factory.Load(m_cont);
}

//////////////////////////////////////////////////////////////////////////////////
// Index implementation
//////////////////////////////////////////////////////////////////////////////////

bool Index::GetVersion(LocalCountryFile const & localFile, MwmInfo & info) const
{
  MwmValue value(localFile);

  feature::DataHeader const & h = value.GetHeader();
  if (!h.IsMWMSuitable())
    return false;

  info.m_limitRect = h.GetBounds();

  pair<int, int> const scaleR = h.GetScaleRange();
  info.m_minScale = static_cast<uint8_t>(scaleR.first);
  info.m_maxScale = static_cast<uint8_t>(scaleR.second);
  info.m_version = value.GetMwmVersion();

  return true;
}

MwmSet::TMwmValueBasePtr Index::CreateValue(LocalCountryFile const & localFile) const
{
  TMwmValueBasePtr p(new MwmValue(localFile));
  ASSERT(static_cast<MwmValue &>(*p.get()).GetHeader().IsMWMSuitable(), ());
  return p;
}

pair<MwmSet::MwmLock, bool> Index::RegisterMap(LocalCountryFile const & localFile)
{
  pair<MwmSet::MwmLock, bool> result = Register(localFile);
  if (result.first.IsLocked() && result.second)
    m_observers.ForEach(&Observer::OnMapRegistered, localFile);
  return result;
}

bool Index::DeregisterMap(CountryFile const & countryFile) { return Deregister(countryFile); }

bool Index::AddObserver(Observer & observer) { return m_observers.Add(observer); }

bool Index::RemoveObserver(Observer const & observer) { return m_observers.Remove(observer); }

void Index::OnMwmDeregistered(LocalCountryFile const & localFile)
{
  m_observers.ForEach(&Observer::OnMapDeregistered, localFile);
}

//////////////////////////////////////////////////////////////////////////////////
// Index::FeaturesLoaderGuard implementation
//////////////////////////////////////////////////////////////////////////////////

Index::FeaturesLoaderGuard::FeaturesLoaderGuard(Index const & parent, MwmId id)
    : m_lock(const_cast<Index &>(parent), id),
      /// @note This guard is suitable when mwm is loaded
      m_vector(m_lock.GetValue<MwmValue>()->m_cont, m_lock.GetValue<MwmValue>()->GetHeader())
{
}

string Index::FeaturesLoaderGuard::GetCountryFileName() const
{
  if (!m_lock.IsLocked())
    return string();
  return m_lock.GetValue<MwmValue>()->GetCountryFile().GetNameWithoutExt();
}

bool Index::FeaturesLoaderGuard::IsWorld() const
{
  return m_lock.GetValue<MwmValue>()->GetHeader().GetType() == feature::DataHeader::world;
}

void Index::FeaturesLoaderGuard::GetFeature(uint32_t offset, FeatureType & ft)
{
  m_vector.Get(offset, ft);
  ft.SetID(FeatureID(m_lock.GetId(), offset));
}
