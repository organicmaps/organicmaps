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

MwmValue::MwmValue(LocalCountryFile const & localFile)
  : m_cont(platform::GetCountryReader(localFile, MapOptions::Map)), m_file(localFile)
{
  m_factory.Load(m_cont);

  auto const version = GetHeader().GetFormat();
  if (version < version::Format::v5)
    ;
  else if (version == version::Format::v5)
    m_table = feature::FeaturesOffsetsTable::CreateIfNotExistsAndLoad(m_file, m_cont);
  else
    m_table = feature::FeaturesOffsetsTable::Load(m_cont);
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

  auto info = make_unique<MwmInfo>();
  info->m_limitRect = h.GetBounds();

  pair<int, int> const scaleR = h.GetScaleRange();
  info->m_minScale = static_cast<uint8_t>(scaleR.first);
  info->m_maxScale = static_cast<uint8_t>(scaleR.second);
  info->m_version = value.GetMwmVersion();

  return info;
}

unique_ptr<MwmSet::MwmValueBase> Index::CreateValue(MwmInfo & info) const
{
  // Create a section with rank table if it does not exist.
  platform::LocalCountryFile const & localFile = info.GetLocalFile();
  unique_ptr<MwmValue> p(new MwmValue(localFile));
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

Index::FeaturesLoaderGuard::FeaturesLoaderGuard(Index const & parent, MwmId const & id)
  : m_handle(parent.GetMwmHandleById(id))
  ,
  /// @note This guard is suitable when mwm is loaded
  m_vector(m_handle.GetValue<MwmValue>()->m_cont, m_handle.GetValue<MwmValue>()->GetHeader(),
           m_handle.GetValue<MwmValue>()->m_table.get())
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

void Index::FeaturesLoaderGuard::GetFeatureByIndex(uint32_t index, FeatureType & ft) const
{
  MwmId const & id = m_handle.GetId();
  ASSERT_NOT_EQUAL(osm::Editor::FeatureStatus::Deleted, m_editor.GetFeatureStatus(id, index),
                   ("Deleted feature was cached. It should not be here. Please review your code."));
  if (!m_editor.Instance().GetEditedFeature(id, index, ft))
    GetOriginalFeatureByIndex(index, ft);
}

void Index::FeaturesLoaderGuard::GetOriginalFeatureByIndex(uint32_t index, FeatureType & ft) const
{
  m_vector.GetByIndex(index, ft);
  ft.SetID(FeatureID(m_handle.GetId(), index));
}
