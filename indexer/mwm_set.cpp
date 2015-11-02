#include "indexer/mwm_set.hpp"
#include "indexer/scales.hpp"

#include "defines.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/stl_add.hpp"

#include "std/algorithm.hpp"
#include "std/sstream.hpp"


using platform::CountryFile;
using platform::LocalCountryFile;

MwmInfo::MwmInfo() : m_minScale(0), m_maxScale(0), m_status(STATUS_DEREGISTERED), m_numRefs(0) {}

MwmInfo::MwmTypeT MwmInfo::GetType() const
{
  if (m_minScale > 0)
    return COUNTRY;
  if (m_maxScale == scales::GetUpperWorldScale())
    return WORLD;
  ASSERT_EQUAL(m_maxScale, scales::GetUpperScale(), ());
  return COASTS;
}

string DebugPrint(MwmSet::MwmId const & id)
{
  ostringstream ss;
  if (id.m_info.get())
    ss << "MwmId [" << id.m_info->GetCountryName() << "]";
  else
    ss << "MwmId [invalid]";
  return ss.str();
}

MwmSet::MwmHandle::MwmHandle() : m_mwmSet(nullptr), m_mwmId(), m_value(nullptr) {}

MwmSet::MwmHandle::MwmHandle(MwmSet & mwmSet, MwmId const & mwmId,
                             unique_ptr<MwmSet::MwmValueBase> && value)
  : m_mwmSet(&mwmSet), m_mwmId(mwmId), m_value(move(value))
{
}

MwmSet::MwmHandle::MwmHandle(MwmHandle && handle)
  : m_mwmSet(handle.m_mwmSet), m_mwmId(handle.m_mwmId), m_value(move(handle.m_value))
{
  handle.m_mwmSet = nullptr;
  handle.m_mwmId.Reset();
  handle.m_value = nullptr;
}

MwmSet::MwmHandle::~MwmHandle()
{
  if (m_mwmSet && m_value)
    m_mwmSet->UnlockValue(m_mwmId, move(m_value));
}

shared_ptr<MwmInfo> const & MwmSet::MwmHandle::GetInfo() const
{
  ASSERT(IsAlive(), ("MwmHandle is not active."));
  return m_mwmId.GetInfo();
}

MwmSet::MwmHandle & MwmSet::MwmHandle::operator=(MwmHandle && handle)
{
  swap(m_mwmSet, handle.m_mwmSet);
  swap(m_mwmId, handle.m_mwmId);
  swap(m_value, handle.m_value);
  return *this;
}

MwmSet::MwmId MwmSet::GetMwmIdByCountryFileImpl(CountryFile const & countryFile) const
{
  string const & name = countryFile.GetNameWithoutExt();
  ASSERT(!name.empty(), ());
  auto const it = m_info.find(name);
  if (it == m_info.cend() || it->second.empty())
    return MwmId();
  return MwmId(it->second.back());
}

pair<MwmSet::MwmId, MwmSet::RegResult> MwmSet::Register(LocalCountryFile const & localFile)
{
  lock_guard<mutex> lock(m_lock);

  CountryFile const & countryFile = localFile.GetCountryFile();
  MwmId const id = GetMwmIdByCountryFileImpl(countryFile);
  if (!id.IsAlive())
    return RegisterImpl(localFile);

  shared_ptr<MwmInfo> info = id.GetInfo();

  // Deregister old mwm for the country.
  if (info->GetVersion() < localFile.GetVersion())
  {
    DeregisterImpl(id);
    return RegisterImpl(localFile);
  }

  string const name = countryFile.GetNameWithoutExt();
  // Update the status of the mwm with the same version.
  if (info->GetVersion() == localFile.GetVersion())
  {
    LOG(LINFO, ("Updating already registered mwm:", name));
    info->SetStatus(MwmInfo::STATUS_REGISTERED);
    info->m_file = localFile;
    return make_pair(id, RegResult::VersionAlreadyExists);
  }

  LOG(LWARNING, ("Trying to add too old (", localFile.GetVersion(), ") mwm (", name,
                 "), current version:", info->GetVersion()));
  return make_pair(MwmId(), RegResult::VersionTooOld);
}

pair<MwmSet::MwmId, MwmSet::RegResult> MwmSet::RegisterImpl(LocalCountryFile const & localFile)
{
  // This function can throw an exception for a bad mwm file.
  shared_ptr<MwmInfo> info(CreateInfo(localFile));
  if (!info)
    return make_pair(MwmId(), RegResult::UnsupportedFileFormat);

  info->m_file = localFile;
  info->SetStatus(MwmInfo::STATUS_REGISTERED);
  m_info[localFile.GetCountryName()].push_back(info);

  return make_pair(MwmId(info), RegResult::Success);
}

bool MwmSet::DeregisterImpl(MwmId const & id)
{
  if (!id.IsAlive())
    return false;

  shared_ptr<MwmInfo> const & info = id.GetInfo();
  if (info->m_numRefs == 0)
  {
    info->SetStatus(MwmInfo::STATUS_DEREGISTERED);
    vector<shared_ptr<MwmInfo>> & infos = m_info[info->GetCountryName()];
    infos.erase(remove(infos.begin(), infos.end(), info), infos.end());
    OnMwmDeregistered(info->GetLocalFile());
    return true;
  }

  info->SetStatus(MwmInfo::STATUS_MARKED_TO_DEREGISTER);
  return false;
}

bool MwmSet::Deregister(CountryFile const & countryFile)
{
  lock_guard<mutex> lock(m_lock);
  return DeregisterImpl(countryFile);
}

bool MwmSet::DeregisterImpl(CountryFile const & countryFile)
{
  MwmId const id = GetMwmIdByCountryFileImpl(countryFile);
  if (!id.IsAlive())
    return false;
  bool const deregistered = DeregisterImpl(id);
  ClearCache(id);
  return deregistered;
}

bool MwmSet::IsLoaded(CountryFile const & countryFile) const
{
  lock_guard<mutex> lock(m_lock);

  MwmId const id = GetMwmIdByCountryFileImpl(countryFile);
  return id.IsAlive() && id.GetInfo()->IsRegistered();
}

void MwmSet::GetMwmsInfo(vector<shared_ptr<MwmInfo>> & info) const
{
  lock_guard<mutex> lock(m_lock);
  info.clear();
  info.reserve(m_info.size());
  for (auto const & p : m_info)
  {
    if (!p.second.empty())
      info.push_back(p.second.back());
  }
}

unique_ptr<MwmSet::MwmValueBase> MwmSet::LockValue(MwmId const & id)
{
  lock_guard<mutex> lock(m_lock);
  return LockValueImpl(id);
}

unique_ptr<MwmSet::MwmValueBase> MwmSet::LockValueImpl(MwmId const & id)
{
  CHECK(id.IsAlive(), (id));
  shared_ptr<MwmInfo> info = id.GetInfo();

  // It's better to return valid "value pointer" even for "out-of-date" files,
  // because they can be locked for a long time by other algos.
  //if (!info->IsUpToDate())
  //  return TMwmValueBasePtr();

  ++info->m_numRefs;

  // Search in cache.
  for (auto it = m_cache.begin(); it != m_cache.end(); ++it)
  {
    if (it->first == id)
    {
      unique_ptr<MwmValueBase> result = move(it->second);
      m_cache.erase(it);
      return result;
    }
  }

  try
  {
    return CreateValue(*info);
  }
  catch (exception const & ex)
  {
    LOG(LERROR, ("Can't create MWMValue for", info->GetCountryName(), "Reason", ex.what()));

    --info->m_numRefs;
    DeregisterImpl(id);
    return nullptr;
  }
}

void MwmSet::UnlockValue(MwmId const & id, unique_ptr<MwmValueBase> && p)
{
  lock_guard<mutex> lock(m_lock);
  UnlockValueImpl(id, move(p));
}

void MwmSet::UnlockValueImpl(MwmId const & id, unique_ptr<MwmValueBase> && p)
{
  ASSERT(id.IsAlive() && p, (id));
  if (!id.IsAlive() || !p)
    return;

  shared_ptr<MwmInfo> const & info = id.GetInfo();
  ASSERT_GREATER(info->m_numRefs, 0, ());
  --info->m_numRefs;
  if (info->m_numRefs == 0 && info->GetStatus() == MwmInfo::STATUS_MARKED_TO_DEREGISTER)
    VERIFY(DeregisterImpl(id), ());

  if (info->IsUpToDate())
  {
    /// @todo Probably, it's better to store only "unique by id" free caches here.
    /// But it's no obvious if we have many threads working with the single mwm.

    m_cache.push_back(make_pair(id, move(p)));
    if (m_cache.size() > m_cacheSize)
    {
      ASSERT_EQUAL(m_cache.size(), m_cacheSize + 1, ());
      m_cache.pop_front();
    }
  }
}

void MwmSet::Clear()
{
  lock_guard<mutex> lock(m_lock);
  ClearCacheImpl(m_cache.begin(), m_cache.end());
  m_info.clear();
}

void MwmSet::ClearCache()
{
  lock_guard<mutex> lock(m_lock);
  ClearCacheImpl(m_cache.begin(), m_cache.end());
}

MwmSet::MwmId MwmSet::GetMwmIdByCountryFile(CountryFile const & countryFile) const
{
  lock_guard<mutex> lock(m_lock);
  return GetMwmIdByCountryFileImpl(countryFile);
}

MwmSet::MwmHandle MwmSet::GetMwmHandleByCountryFile(CountryFile const & countryFile)
{
  lock_guard<mutex> lock(m_lock);
  return GetMwmHandleByIdImpl(GetMwmIdByCountryFileImpl(countryFile));
}

MwmSet::MwmHandle MwmSet::GetMwmHandleById(MwmId const & id)
{
  lock_guard<mutex> lock(m_lock);
  return GetMwmHandleByIdImpl(id);
}

MwmSet::MwmHandle MwmSet::GetMwmHandleByIdImpl(MwmId const & id)
{
  unique_ptr<MwmValueBase> value;
  if (id.IsAlive())
    value = LockValueImpl(id);
  return MwmHandle(*this, id, move(value));
}

void MwmSet::ClearCacheImpl(CacheType::iterator beg, CacheType::iterator end)
{
  m_cache.erase(beg, end);
}

void MwmSet::ClearCache(MwmId const & id)
{
  auto sameId = [&id](pair<MwmSet::MwmId, unique_ptr<MwmSet::MwmValueBase>> const & p)
  {
    return (p.first == id);
  };
  ClearCacheImpl(RemoveIfKeepValid(m_cache.begin(), m_cache.end(), sameId), m_cache.end());
}

string DebugPrint(MwmSet::RegResult result)
{
  switch (result)
  {
    case MwmSet::RegResult::Success:
      return "Success";
    case MwmSet::RegResult::VersionAlreadyExists:
      return "VersionAlreadyExists";
    case MwmSet::RegResult::VersionTooOld:
      return "VersionTooOld";
    case MwmSet::RegResult::BadFile:
      return "BadFile";
    case MwmSet::RegResult::UnsupportedFileFormat:
      return "UnsupportedFileFormat";
  }
}
