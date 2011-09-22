#include "mwm_set.hpp"
#include "../coding/file_container.hpp"
#include "../base/logging.hpp"
#include "../std/algorithm.hpp"

namespace
{
  struct MwmIdIsEqualTo
  {
    MwmSet::MwmId m_id;
    explicit MwmIdIsEqualTo(MwmSet::MwmId id) : m_id(id) {}
    bool operator() (pair<MwmSet::MwmId, FilesContainerR *> const & p) const
    {
      return p.first == m_id;
    }
  };
}  // unnamed namespace

MwmSet::MwmLock::MwmLock(MwmSet const & mwmSet, MwmId mwmId)
  : m_mwmSet(mwmSet), m_id(mwmId), m_pFileContainer(mwmSet.LockContainer(mwmId))
{
  //LOG(LINFO, ("MwmLock::MwmLock()", m_id));
}

MwmSet::MwmLock::~MwmLock()
{
  //LOG(LINFO, ("MwmLock::~MwmLock()", m_id));
  if (m_pFileContainer)
    m_mwmSet.UnlockContainer(m_id, m_pFileContainer);
}

FilesContainerR * MwmSet::MwmLock::GetFileContainer() const
{
  return m_pFileContainer;
}


MwmSet::MwmSet(function<void (string const &, MwmInfo &)> const & fnGetMwmInfo,
               function<FilesContainerR * (string const &)> const & fnCreateContainer,
               size_t cacheSize)
  : m_cacheSize(cacheSize), m_fnGetMwmInfo(fnGetMwmInfo), m_fnCreateContainer(fnCreateContainer)
{
  //LOG(LINFO, ("MwmSet::MwmSet()"));
}

MwmSet::~MwmSet()
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  //LOG(LINFO, ("MwmSet::~MwmSet()"));

  ClearCacheImpl(m_cache.begin(), m_cache.end());

  for (size_t i = 0; i < m_info.size(); ++i)
  {
    if (m_info[i].m_status == MwmInfo::STATUS_ACTIVE)
    {
      ASSERT_EQUAL(m_info[i].m_lockCount, 0, (i, m_name[i]));
      ASSERT_NOT_EQUAL(m_name[i], string(), (i));
    }
  }
}

inline void MwmSet::UpdateMwmInfo(MwmInfo & info)
{
  if (info.m_status == MwmInfo::STATUS_TO_REMOVE && info.m_lockCount == 0)
    info.m_status = MwmInfo::STATUS_REMOVED;
}

MwmSet::MwmId MwmSet::GetFreeId()
{
  MwmId const size = static_cast<MwmId>(m_info.size());
  for (MwmId i = 0; i < size; ++i)
  {
    if (m_info[i].m_status == MwmInfo::STATUS_REMOVED)
      return i;
  }
  m_info.push_back(MwmInfo());
  m_info.back().m_status = MwmInfo::STATUS_REMOVED;
  m_info.back().m_lockCount = 0;
  m_name.push_back(string());
  return size;
}

MwmSet::MwmId MwmSet::GetIdByName(string const & name)
{
  MwmId const size = static_cast<MwmId>(m_info.size());
  for (MwmId i = 0; i < size; ++i)
  {
    UpdateMwmInfo(m_info[i]);
    if (m_info[i].m_status == MwmInfo::STATUS_ACTIVE && m_name[i] == name)
      return i;
  }
  return INVALID_MWM_ID;
}

bool MwmSet::Add(string const & fileName)
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  //LOG(LINFO, ("MwmSet::Add()", fileName));

  if (GetIdByName(fileName) != INVALID_MWM_ID)
    return false;

  MwmId const id = GetFreeId();
  m_name[id] = fileName;
  memset(&m_info[id], 0, sizeof(MwmInfo));
  m_fnGetMwmInfo(fileName, m_info[id]);
  m_info[id].m_lockCount = 0;
  m_info[id].m_status = MwmInfo::STATUS_ACTIVE;
  return true;
}

void MwmSet::Remove(string const & fileName)
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  //LOG(LINFO, ("MwmSet::Remove()", fileName));

  MwmId const id = GetIdByName(fileName);
  if (id != INVALID_MWM_ID)
  {
    if (m_info[id].m_lockCount == 0)
      m_info[id].m_status = MwmInfo::STATUS_REMOVED;
    else
      m_info[id].m_status = MwmInfo::STATUS_TO_REMOVE;
    m_name[id].clear();

    // Update the cache.
    ClearCacheImpl(remove_if(m_cache.begin(), m_cache.end(), MwmIdIsEqualTo(id)), m_cache.end());
  }
}

void MwmSet::GetMwmInfo(vector<MwmInfo> & info) const
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  for (vector<MwmInfo>::iterator it = m_info.begin(); it != m_info.end(); ++it)
    UpdateMwmInfo(*it);

  info = m_info;
}

FilesContainerR * MwmSet::LockContainer(MwmId id) const
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  //LOG(LINFO, ("MwmSet::LockContainer()", id));

  ASSERT_LESS(id, m_info.size(), ());
  if (id >= m_info.size())
    return NULL;

  UpdateMwmInfo(m_info[id]);
  if (m_info[id].m_status != MwmInfo::STATUS_ACTIVE)
    return NULL;

  ++m_info[id].m_lockCount;

  // Search in cache.
  for (CacheType::iterator it = m_cache.begin(); it != m_cache.end(); ++it)
  {
    if (it->first == id)
    {
      FilesContainerR * result = it->second;
      m_cache.erase(it);
      return result;
    }
  }
  return m_fnCreateContainer(m_name[id]);
}

void MwmSet::UnlockContainer(MwmId id, FilesContainerR * pContainer) const
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  //LOG(LINFO, ("MwmSet::UnlockContainer()", id));

  ASSERT(pContainer, (id));
  ASSERT_LESS(id, m_info.size(), ());
  if (id >= m_info.size() || !pContainer)
    return;

  ASSERT_GREATER(m_info[id].m_lockCount, 0, ());
  if (m_info[id].m_lockCount > 0)
    --m_info[id].m_lockCount;
  UpdateMwmInfo(m_info[id]);

  if (m_info[id].m_status == MwmInfo::STATUS_ACTIVE)
  {
    m_cache.push_back(make_pair(id, pContainer));
    if (m_cache.size() > m_cacheSize)
    {
      ASSERT_EQUAL(m_cache.size(), m_cacheSize + 1, ());
      delete m_cache.front().second;
      m_cache.pop_front();
    }
  }
  else
    delete pContainer;
}

void MwmSet::ClearCache()
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  ClearCacheImpl(m_cache.begin(), m_cache.end());
}

void MwmSet::ClearCacheImpl(CacheType::iterator beg, CacheType::iterator end)
{
  for (CacheType::iterator it = beg; it != end; ++it)
    delete it->second;
  m_cache.erase(beg, end);
}
