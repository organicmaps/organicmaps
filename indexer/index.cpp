#include "indexer/index.hpp"

#include "platform/local_country_file_utils.hpp"

#include "indexer/rank_table.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;

//////////////////////////////////////////////////////////////////////////////////
// MwmValue implementation
//////////////////////////////////////////////////////////////////////////////////
using namespace std;

MwmValue::MwmValue(LocalCountryFile const & localFile)
  : m_cont(platform::GetCountryReader(localFile, MapOptions::Map)), m_file(localFile)
{
  m_factory.Load(m_cont);
}

void MwmValue::SetTable(MwmInfoEx & info)
{
  auto const version = GetHeader().GetFormat();
  if (version < version::Format::v5)
    return;

  m_table = info.m_table.lock();
  if (!m_table)
  {
    if (version == version::Format::v5)
      m_table = feature::FeaturesOffsetsTable::CreateIfNotExistsAndLoad(m_file, m_cont);
    else
      m_table = feature::FeaturesOffsetsTable::Load(m_cont);
    info.m_table = m_table;
  }
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

  auto info = make_unique<MwmInfoEx>();
  info->m_limitRect = h.GetBounds();

  pair<int, int> const scaleR = h.GetScaleRange();
  info->m_minScale = static_cast<uint8_t>(scaleR.first);
  info->m_maxScale = static_cast<uint8_t>(scaleR.second);
  info->m_version = value.GetMwmVersion();
  // Copying to drop the const qualifier.
  feature::RegionData regionData(value.GetRegionData());
  info->m_data = regionData;

  return unique_ptr<MwmInfo>(move(info));
}

unique_ptr<MwmSet::MwmValueBase> Index::CreateValue(MwmInfo & info) const
{
  // Create a section with rank table if it does not exist.
  platform::LocalCountryFile const & localFile = info.GetLocalFile();
  unique_ptr<MwmValue> p(new MwmValue(localFile));
  p->SetTable(dynamic_cast<MwmInfoEx &>(info));
  ASSERT(p->GetHeader().IsMWMSuitable(), ());
  return unique_ptr<MwmSet::MwmValueBase>(move(p));
}

pair<MwmSet::MwmId, MwmSet::RegResult> Index::RegisterMap(LocalCountryFile const & localFile)
{
  return Register(localFile);
}

bool Index::DeregisterMap(CountryFile const & countryFile) { return Deregister(countryFile); }

//////////////////////////////////////////////////////////////////////////////////
// Index::FeaturesLoaderGuard implementation
//////////////////////////////////////////////////////////////////////////////////

Index::FeaturesLoaderGuard::FeaturesLoaderGuard(Index const & index, MwmId const & id)
  : m_handle(index.GetMwmHandleById(id))
{
  if (!m_handle.IsAlive())
    return;

  auto const & value = *m_handle.GetValue<MwmValue>();
  m_vector = make_unique<FeaturesVector>(value.m_cont, value.GetHeader(), value.m_table.get());
}

string Index::FeaturesLoaderGuard::GetCountryFileName() const
{
  if (!m_handle.IsAlive())
    return string();

  return m_handle.GetValue<MwmValue>()->GetCountryFileName();
}

bool Index::FeaturesLoaderGuard::IsWorld() const
{
  if (!m_handle.IsAlive())
    return false;

  return m_handle.GetValue<MwmValue>()->GetHeader().GetType() == feature::DataHeader::world;
}

unique_ptr<FeatureType> Index::FeaturesLoaderGuard::GetOriginalFeatureByIndex(uint32_t index) const
{
  auto feature = make_unique<FeatureType>();
  if (!GetOriginalFeatureByIndex(index, *feature))
    return {};

  return feature;
}

unique_ptr<FeatureType> Index::FeaturesLoaderGuard::GetOriginalOrEditedFeatureByIndex(uint32_t index) const
{
  auto feature = make_unique<FeatureType>();
  if (!m_handle.IsAlive())
    return {};

  ASSERT_NOT_EQUAL(m_editor.GetFeatureStatus(m_handle.GetId(), index), osm::Editor::FeatureStatus::Created, ());
  if (!GetFeatureByIndex(index, *feature))
    return {};

  return feature;
}

bool Index::FeaturesLoaderGuard::GetFeatureByIndex(uint32_t index, FeatureType & ft) const
{
  if (!m_handle.IsAlive())
    return false;

  MwmId const & id = m_handle.GetId();
  ASSERT_NOT_EQUAL(osm::Editor::FeatureStatus::Deleted, m_editor.GetFeatureStatus(id, index),
                   ("Deleted feature was cached. It should not be here. Please review your code."));
  if (m_editor.Instance().GetEditedFeature(id, index, ft))
    return true;
  return GetOriginalFeatureByIndex(index, ft);
}

bool Index::FeaturesLoaderGuard::GetOriginalFeatureByIndex(uint32_t index, FeatureType & ft) const
{
  if (!m_handle.IsAlive())
    return false;

  ASSERT(m_vector != nullptr, ());
  m_vector->GetByIndex(index, ft);
  ft.SetID(FeatureID(m_handle.GetId(), index));
  return true;
}

size_t Index::FeaturesLoaderGuard::GetNumFeatures() const
{
  if (!m_handle.IsAlive())
    return 0;

  ASSERT(m_vector.get(), ());
  return m_vector->GetNumFeatures();
}
