#include "indexer/index.hpp"

#include "platform/local_country_file_utils.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;

//////////////////////////////////////////////////////////////////////////////////
// MwmValue implementation
//////////////////////////////////////////////////////////////////////////////////

MwmValue::MwmValue(LocalCountryFile const & localFile)
    : m_cont(platform::GetCountryReader(localFile, MapOptions::Map)),
      m_file(localFile),
      m_table(0)
{
  m_factory.Load(m_cont);
}

void MwmValue::SetTable(MwmInfoEx & info)
{
  auto const version = GetHeader().GetFormat();
  if (version < version::v5)
    return;

  if (!info.m_table)
  {
    if (version == version::v5)
      info.m_table = feature::FeaturesOffsetsTable::CreateIfNotExistsAndLoad(m_file, m_cont);
    else
      info.m_table = feature::FeaturesOffsetsTable::Load(m_cont);
  }
  m_table = info.m_table.get();
}

//////////////////////////////////////////////////////////////////////////////////
// Index implementation
//////////////////////////////////////////////////////////////////////////////////

unique_ptr<MwmInfo> Index::CreateInfo(platform::LocalCountryFile const & localFile) const
{
  MwmValue value(localFile);

  feature::DataHeader const & h = value.GetHeader();
  if (!h.IsMWMSuitable())
    return nullptr;

  unique_ptr<MwmInfoEx> info(new MwmInfoEx());
  info->m_limitRect = h.GetBounds();

  pair<int, int> const scaleR = h.GetScaleRange();
  info->m_minScale = static_cast<uint8_t>(scaleR.first);
  info->m_maxScale = static_cast<uint8_t>(scaleR.second);
  info->m_version = value.GetMwmVersion();

  return unique_ptr<MwmInfo>(move(info));
}

unique_ptr<MwmSet::MwmValueBase> Index::CreateValue(MwmInfo & info) const
{
  unique_ptr<MwmValue> p(new MwmValue(info.GetLocalFile()));
  p->SetTable(dynamic_cast<MwmInfoEx &>(info));
  ASSERT(p->GetHeader().IsMWMSuitable(), ());
  return unique_ptr<MwmSet::MwmValueBase>(move(p));
}

pair<MwmSet::MwmId, MwmSet::RegResult> Index::RegisterMap(LocalCountryFile const & localFile)
{
  auto result = Register(localFile);
  if (result.first.IsAlive() && result.second == MwmSet::RegResult::Success)
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
    : m_handle(parent.GetMwmHandleById(id)),
      /// @note This guard is suitable when mwm is loaded
      m_vector(m_handle.GetValue<MwmValue>()->m_cont,
               m_handle.GetValue<MwmValue>()->GetHeader(),
               m_handle.GetValue<MwmValue>()->m_table)
{
}

string Index::FeaturesLoaderGuard::GetCountryFileName() const
{
  if (!m_handle.IsAlive())
    return string();
  return m_handle.GetValue<MwmValue>()->GetCountryFileName();
}

bool Index::FeaturesLoaderGuard::IsWorld() const
{
  return m_handle.GetValue<MwmValue>()->GetHeader().GetType() == feature::DataHeader::world;
}

void Index::FeaturesLoaderGuard::GetFeatureByIndex(uint32_t index, FeatureType & ft)
{
  m_vector.GetByIndex(index, ft);
  ft.SetID(FeatureID(m_handle.GetId(), index));
}
