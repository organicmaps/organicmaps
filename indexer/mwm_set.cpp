#include "indexer/mwm_set.hpp"
#include "indexer/scales.hpp"

#include "defines.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_add.hpp"

#include "std/algorithm.hpp"

MwmInfo::MwmInfo() : m_minScale(0), m_maxScale(0), m_status(STATUS_DEREGISTERED), m_lockCount(0) {}

MwmInfo::MwmTypeT MwmInfo::GetType() const
{
  if (m_minScale > 0)
    return COUNTRY;
  if (m_maxScale == scales::GetUpperWorldScale())
    return WORLD;
  ASSERT_EQUAL(m_maxScale, scales::GetUpperScale(), ());
  return COASTS;
}

MwmSet::MwmLock::MwmLock() : m_mwmSet(nullptr), m_mwmId(), m_value(nullptr) {}

MwmSet::MwmLock::MwmLock(MwmSet & mwmSet, MwmId const & mwmId)
    : m_mwmSet(&mwmSet), m_mwmId(mwmId), m_value(m_mwmSet->LockValue(m_mwmId))
{
}

MwmSet::MwmLock::MwmLock(MwmSet & mwmSet, MwmId const & mwmId, TMwmValueBasePtr value)
    : m_mwmSet(&mwmSet), m_mwmId(mwmId), m_value(value)
{
}

MwmSet::MwmLock::MwmLock(MwmLock && lock)
    : m_mwmSet(lock.m_mwmSet), m_mwmId(lock.m_mwmId), m_value(move(lock.m_value))
{
  lock.m_mwmSet = nullptr;
  lock.m_mwmId.Reset();
}

MwmSet::MwmLock::~MwmLock()
{
  if (m_mwmSet && m_value)
    m_mwmSet->UnlockValue(m_mwmId, m_value);
}

shared_ptr<MwmInfo> const & MwmSet::MwmLock::GetInfo() const
{
  ASSERT(IsLocked(), ("MwmLock is not active."));
  return m_mwmId.GetInfo();
}

MwmSet::MwmLock & MwmSet::MwmLock::operator=(MwmLock && lock)
{
  swap(m_mwmSet, lock.m_mwmSet);
  swap(m_mwmId, lock.m_mwmId);
  swap(m_value, lock.m_value);
  return *this;
}

MwmSet::MwmSet(size_t cacheSize)
  : m_cacheSize(cacheSize)
{
}

MwmSet::~MwmSet()
{
  // Need do call Cleanup() in derived class.
  ASSERT(m_cache.empty(), ());
}

void MwmSet::Cleanup()
{
  lock_guard<mutex> lock(m_lock);

  ClearCacheImpl(m_cache.begin(), m_cache.end());

#ifdef DEBUG
  for (auto const & p : m_info)
  {
    MwmInfo const & info = *p.second;
    if (info.IsUpToDate())
    {
      ASSERT_EQUAL(info.m_lockCount, 0, (info.m_fileName));
      ASSERT(!info.m_fileName.empty(), ());
    }
  }
#endif
}

MwmSet::MwmId MwmSet::GetMwmIdByFileNameImpl(TMwmFileName const & name) const
{
  ASSERT(!name.empty(), ());
  auto const it = m_info.find(name);
  if (it == m_info.cend())
    return MwmId();
  return MwmId(it->second);
}

pair<MwmSet::MwmLock, bool> MwmSet::Register(TMwmFileName const & fileName)
{
  lock_guard<mutex> lock(m_lock);

  MwmId const id = GetMwmIdByFileNameImpl(fileName);
  if (!id.IsAlive())
    return RegisterImpl(fileName);
  shared_ptr<MwmInfo> info = id.GetInfo();
  if (info->IsRegistered())
    LOG(LWARNING, ("Trying to add already registered mwm:", fileName));
  else
    info->SetStatus(MwmInfo::STATUS_UP_TO_DATE);
  return make_pair(GetLock(id), false);
}

pair<MwmSet::MwmLock, bool> MwmSet::RegisterImpl(TMwmFileName const & fileName)
{
  shared_ptr<MwmInfo> info(new MwmInfo());

  // This function can throw an exception for a bad mwm file.
  if (!GetVersion(fileName, *info))
    return make_pair(MwmLock(), false);
  info->SetStatus(MwmInfo::STATUS_UP_TO_DATE);
  info->m_fileName = fileName;
  m_info[fileName] = info;

  return make_pair(GetLock(MwmId(info)), true);
}

bool MwmSet::DeregisterImpl(MwmId const & id)
{
  if (!id.IsAlive())
    return false;
  shared_ptr<MwmInfo> const & info = id.GetInfo();
  if (info->m_lockCount == 0)
  {
    info->SetStatus(MwmInfo::STATUS_DEREGISTERED);
    m_info.erase(info->m_fileName);
    OnMwmDeleted(info);
    return true;
  }
  info->SetStatus(MwmInfo::STATUS_MARKED_TO_DEREGISTER);
  return false;
}

bool MwmSet::Deregister(TMwmFileName const & fileName)
{
  lock_guard<mutex> lock(m_lock);
  return DeregisterImpl(fileName);
}

bool MwmSet::DeregisterImpl(TMwmFileName const & fileName)
{
  MwmId const id = GetMwmIdByFileNameImpl(fileName);
  if (!id.IsAlive())
    return false;
  bool const deregistered = DeregisterImpl(id);
  ClearCache(id);
  return deregistered;
}

void MwmSet::DeregisterAll()
{
  lock_guard<mutex> lock(m_lock);

  for (auto it = m_info.begin(); it != m_info.end();)
  {
    auto cur = it++;
    DeregisterImpl(MwmId(cur->second));
  }

  // Do not call ClearCache - it's under mutex lock.
  ClearCacheImpl(m_cache.begin(), m_cache.end());
}

bool MwmSet::IsLoaded(TMwmFileName const & fileName) const
{
  lock_guard<mutex> lock(m_lock);

  MwmId const id = GetMwmIdByFileNameImpl(fileName + DATA_FILE_EXTENSION);
  return id.IsAlive() && id.GetInfo()->IsRegistered();
}

void MwmSet::GetMwmsInfo(vector<shared_ptr<MwmInfo>> & info) const
{
  lock_guard<mutex> lock(m_lock);
  info.clear();
  info.reserve(m_info.size());
  for (auto const & p : m_info)
    info.push_back(p.second);
}

MwmSet::TMwmValueBasePtr MwmSet::LockValue(MwmId const & id)
{
  lock_guard<mutex> lock(m_lock);
  return LockValueImpl(id);
}

MwmSet::TMwmValueBasePtr MwmSet::LockValueImpl(MwmId const & id)
{
  CHECK(id.IsAlive(), (id));
  shared_ptr<MwmInfo> info = id.GetInfo();
  if (!info->IsUpToDate())
    return TMwmValueBasePtr();

  ++info->m_lockCount;

  // Search in cache.
  for (CacheType::iterator it = m_cache.begin(); it != m_cache.end(); ++it)
  {
    if (it->first == id)
    {
      TMwmValueBasePtr result = it->second;
      m_cache.erase(it);
      return result;
    }
  }
  return CreateValue(info->m_fileName);
}

void MwmSet::UnlockValue(MwmId const & id, TMwmValueBasePtr p)
{
  lock_guard<mutex> lock(m_lock);
  UnlockValueImpl(id, p);
}

void MwmSet::UnlockValueImpl(MwmId const & id, TMwmValueBasePtr p)
{
  ASSERT(id.IsAlive(), ());
  ASSERT(p.get() != nullptr, (id));
  if (!id.IsAlive() || p.get() == nullptr)
    return;

  shared_ptr<MwmInfo> const & info = id.GetInfo();
  CHECK_GREATER(info->m_lockCount, 0, ());
  --info->m_lockCount;
  if (info->m_lockCount == 0)
  {
    switch (info->GetStatus())
    {
      case MwmInfo::STATUS_MARKED_TO_DEREGISTER:
        CHECK(DeregisterImpl(id), ());
        break;
      case MwmInfo::STATUS_PENDING_UPDATE:
        OnMwmReadyForUpdate(info);
        break;
      default:
        break;
    }
  }

  if (info->IsUpToDate())
  {
    m_cache.push_back(make_pair(id, p));
    if (m_cache.size() > m_cacheSize)
    {
      ASSERT_EQUAL(m_cache.size(), m_cacheSize + 1, ());
      m_cache.pop_front();
    }
  }
}

void MwmSet::ClearCache()
{
  lock_guard<mutex> lock(m_lock);
  ClearCacheImpl(m_cache.begin(), m_cache.end());
}

MwmSet::MwmId MwmSet::GetMwmIdByFileName(TMwmFileName const & fileName) const
{
  lock_guard<mutex> lock(m_lock);

  MwmId const id = GetMwmIdByFileNameImpl(fileName);
  ASSERT(id.IsAlive(), ("Can't get an mwm's (", fileName, ") identifier."));
  return id;
}

MwmSet::MwmLock MwmSet::GetMwmLockByFileName(TMwmFileName const & fileName)
{
  lock_guard<mutex> lock(m_lock);

  MwmId const id = GetMwmIdByFileNameImpl(fileName);
  TMwmValueBasePtr value(nullptr);
  if (id.IsAlive())
    value = LockValueImpl(id);
  return MwmLock(*this, id, value);
}

void MwmSet::ClearCacheImpl(CacheType::iterator beg, CacheType::iterator end)
{
  m_cache.erase(beg, end);
}

namespace
{
  struct MwmIdIsEqualTo
  {
    MwmSet::MwmId m_id;

    explicit MwmIdIsEqualTo(MwmSet::MwmId const & id) : m_id(id) {}

    bool operator()(pair<MwmSet::MwmId, MwmSet::TMwmValueBasePtr> const & p) const
    {
      return p.first == m_id;
    }
  };
}

void MwmSet::ClearCache(MwmId const & id)
{
  ClearCacheImpl(RemoveIfKeepValid(m_cache.begin(), m_cache.end(), MwmIdIsEqualTo(id)), m_cache.end());
}
