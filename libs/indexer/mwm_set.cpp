#include "indexer/mwm_set.hpp"

#include "indexer/features_offsets_table.hpp"
#include "indexer/metadata_serdes.hpp"  // needed for MwmValue dtor
#include "indexer/scales.hpp"

#include "platform/local_country_file_utils.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <sstream>

using platform::CountryFile;
using platform::LocalCountryFile;

MwmInfo::MwmTypeT MwmInfo::GetType() const
{
  if (m_minScale > 0)
    return COUNTRY;
  if (m_maxScale == scales::GetUpperWorldScale())
    return WORLD;
  ASSERT_EQUAL(m_maxScale, scales::GetUpperScale(), ());
  return COASTS;
}

bool MwmInfo::IsAddressLikeUS() const
{
  return GetRegionData().Get(feature::RegionData::RD_ADDRESS_FORMAT) == "us";
}

bool MwmId::IsDeregistered(platform::LocalCountryFile const & deregisteredCountryFile) const
{
  return m_info && m_info->GetStatus() == MwmInfo::STATUS_DEREGISTERED &&
         m_info->GetLocalFile() == deregisteredCountryFile;
}

std::string DebugPrint(MwmId const & id)
{
  std::ostringstream ss;
  if (id.m_info.get())
    ss << "MwmId [" << id.m_info->GetCountryName() << ", " << id.m_info->GetVersion() << "]";
  else
    ss << "MwmId [invalid]";
  return ss.str();
}

MwmId MwmSet::GetMwmIdByCountryFileImpl(CountryFile const & countryFile) const
{
  std::string const & name = countryFile.GetName();
  ASSERT(!name.empty(), ());
  return GetIdByKeyImpl(name);
}

std::pair<MwmSet::MwmId, MwmSet::RegResult> MwmSet::Register(LocalCountryFile const & localFile)
{
  std::pair<MwmSet::MwmId, MwmSet::RegResult> result;
  auto registerFile = [&](EventList & events)
  {
    CountryFile const & countryFile = localFile.GetCountryFile();
    MwmId const id = GetMwmIdByCountryFileImpl(countryFile);
    if (!id.IsAlive())
    {
      result = RegisterImpl(localFile, events);
      return;
    }

    std::shared_ptr<MwmInfo> info = id.GetInfo();

    // Deregister old mwm for the country.
    if (info->GetVersion() < localFile.GetVersion())
    {
      DeregisterImpl(id, events);
      result = RegisterImpl(localFile, events);
      return;
    }

    std::string const name = countryFile.GetName();
    // Update the status of the mwm with the same version.
    if (info->GetVersion() == localFile.GetVersion())
    {
      LOG(LINFO, ("Updating already registered mwm:", name));
      SetStatus(*info, MwmInfo::STATUS_REGISTERED, events);
      info->m_file = localFile;
      result = std::make_pair(id, RegResult::VersionAlreadyExists);
      return;
    }

    LOG(LWARNING, ("Trying to add too old (", localFile.GetVersion(), ") mwm (", name,
                   "), current version:", info->GetVersion()));
    result = std::make_pair(MwmId(), RegResult::VersionTooOld);
    return;
  };

  WithEventLog(registerFile);
  return result;
}

std::pair<MwmSet::MwmId, MwmSet::RegResult> MwmSet::RegisterImpl(LocalCountryFile const & localFile, EventList & events)
{
  // This function can throw an exception for a bad mwm file.
  std::shared_ptr<MwmInfo> info(CreateInfo(localFile));
  if (!info)
    return std::make_pair(MwmId(), RegResult::UnsupportedFileFormat);

  info->m_file = localFile;
  SetStatus(*info, MwmInfo::STATUS_REGISTERED, events);
  // No unique key assert here: an old mwm with "STATUS_MARKED_TO_DEREGISTER" may coexist.
  AddToRegistryImpl(info);

  return std::make_pair(MwmId(info), RegResult::Success);
}

bool MwmSet::Deregister(CountryFile const & countryFile)
{
  bool deregistered = false;
  WithEventLog([&](EventList & events) { deregistered = DeregisterImpl(countryFile, events); });
  return deregistered;
}

bool MwmSet::DeregisterImpl(CountryFile const & countryFile, EventList & events)
{
  MwmId const id = GetMwmIdByCountryFileImpl(countryFile);
  if (!id.IsAlive())
    return false;
  bool const deregistered = DeregisterImpl(id, events);
  ClearCache(id);
  return deregistered;
}

bool MwmSet::IsLoaded(CountryFile const & countryFile) const
{
  std::lock_guard<std::mutex> lock(m_lock);

  MwmId const id = GetMwmIdByCountryFileImpl(countryFile);
  return id.IsAlive() && id.GetInfo()->IsRegistered();
}

std::vector<std::string> MwmSet::GetLoadedCountryNames(m2::RectD const & rect) const
{
  std::lock_guard<std::mutex> lock(m_lock);

  std::vector<std::string> res;
  for (auto const & [name, info] : m_registry)
    if (!info.empty() && info.back()->IsRegistered() && info.back()->m_bordersRect.IsIntersect(rect) &&
        info.back()->GetType() == MwmInfo::COUNTRY)
      res.push_back(name);
  return res;
}

int64_t MwmSet::GetMwmVersion(CountryFile const & countryFile) const
{
  std::lock_guard<std::mutex> lock(m_lock);

  MwmId const id = GetMwmIdByCountryFileImpl(countryFile);
  return id.IsAlive() ? id.GetInfo()->GetVersion() : 0;
}

void MwmSet::SetStatus(MwmInfo & info, MwmInfo::Status status, EventList & events)
{
  MwmInfo::Status oldStatus = info.SetStatus(status);
  if (oldStatus == status)
    return;

  switch (status)
  {
  case MwmInfo::STATUS_REGISTERED: events.Add(Event(Event::TYPE_REGISTERED, info.GetLocalFile())); break;
  case MwmInfo::STATUS_MARKED_TO_DEREGISTER: break;
  case MwmInfo::STATUS_DEREGISTERED: events.Add(Event(Event::TYPE_DEREGISTERED, info.GetLocalFile())); break;
  }
}

void MwmSet::ProcessEvents(EventList & events)
{
  for (auto const & event : events.Get())
  {
    switch (event.m_type)
    {
    case Event::TYPE_REGISTERED: m_observers.ForEach(&Observer::OnMapRegistered, event.m_file); break;
    case Event::TYPE_DEREGISTERED: m_observers.ForEach(&Observer::OnMapDeregistered, event.m_file); break;
    }
  }
}

MwmSet::MwmId MwmSet::GetMwmIdByCountryFile(CountryFile const & countryFile) const
{
  std::lock_guard<std::mutex> lock(m_lock);
  return GetMwmIdByCountryFileImpl(countryFile);
}

MwmSet::MwmHandle MwmSet::GetMwmHandleByCountryFile(CountryFile const & countryFile)
{
  MwmSet::MwmHandle handle;
  WithEventLog([&](EventList & events) { handle = GetHandleByIdImpl(GetMwmIdByCountryFileImpl(countryFile), events); });
  return handle;
}

MwmSet::MwmHandle MwmSet::GetMwmHandleById(MwmId const & id)
{
  MwmSet::MwmHandle handle;
  WithEventLog([&](EventList & events) { handle = GetHandleByIdImpl(id, events); });
  return handle;
}

// MwmValue ----------------------------------------------------------------------------------------

MwmValue::MwmValue(ModelReaderPtr const & reader, LocalCountryFile const & localFile)
  : m_cont(reader)
  , m_file(localFile)
{
  m_version = version::MwmVersion::Read(m_cont);
  if (m_version.GetFormat() < version::Format::v11)
    MYTHROW(CorruptedMwmFile, (m_cont.GetFileName()));

  m_header.Load(m_cont);
}

MwmValue::MwmValue(LocalCountryFile const & localFile)
  : MwmValue(platform::GetCountryReader(localFile, MapFileType::Map), localFile)
{}

MwmValue::~MwmValue() {}

void MwmValue::SetTable(MwmInfoEx & info)
{
  m_ftTable = info.m_ftTable.lock();
  if (!m_ftTable)
  {
    m_ftTable = feature::FeaturesOffsetsTable::Load(m_cont, FEATURE_OFFSETS_FILE_TAG);
    info.m_ftTable = m_ftTable;
  }

  m_relTable = info.m_relTable.lock();
  if (!m_relTable && m_cont.IsExist(RELATION_OFFSETS_FILE_TAG))
  {
    m_relTable = feature::FeaturesOffsetsTable::Load(m_cont, RELATION_OFFSETS_FILE_TAG);
    info.m_relTable = m_relTable;
  }
}

std::string DebugPrint(MwmSet::RegResult result)
{
  switch (result)
  {
  case MwmSet::RegResult::Success: return "Success";
  case MwmSet::RegResult::VersionAlreadyExists: return "VersionAlreadyExists";
  case MwmSet::RegResult::VersionTooOld: return "VersionTooOld";
  case MwmSet::RegResult::BadFile: return "BadFile";
  case MwmSet::RegResult::UnsupportedFileFormat: return "UnsupportedFileFormat";
  }
  UNREACHABLE();
}

std::string DebugPrint(MwmSet::Event::Type type)
{
  switch (type)
  {
  case MwmSet::Event::TYPE_REGISTERED: return "Registered";
  case MwmSet::Event::TYPE_DEREGISTERED: return "Deregistered";
  }
  return "Undefined";
}

std::string DebugPrint(MwmSet::Event const & event)
{
  std::ostringstream os;
  os << "MwmSet::Event [" << DebugPrint(event.m_type) << ", " << DebugPrint(event.m_file) << "]";
  return os.str();
}
