#include "mwm_set.hpp"
#include "scales.hpp"

#include "../../defines.hpp"

#include "../base/logging.hpp"
#include "../base/macros.hpp"
#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"
#include "../std/memcpy.hpp"


MwmInfo::MwmInfo() : m_lockCount(0), m_status(STATUS_REMOVED)
{
  // Important: STATUS_REMOVED - is the default value.
  // Apply STATUS_ACTIVE before adding to maps container.
}

MwmInfo::MwmTypeT MwmInfo::GetType() const
{
  if (m_minScale > 0) return COUNTRY;
  if (m_maxScale == scales::GetUpperWorldScale()) return WORLD;
  ASSERT_EQUAL(m_maxScale, scales::GetUpperScale(), ());
  return COASTS;
}


MwmSet::MwmLock::MwmLock(MwmSet & mwmSet, MwmId mwmId)
  : m_mwmSet(mwmSet), m_id(mwmId), m_pValue(mwmSet.LockValue(mwmId))
{
}

MwmSet::MwmLock::~MwmLock()
{
  if (m_pValue)
    m_mwmSet.UnlockValue(m_id, m_pValue);
}


MwmSet::MwmSet(size_t cacheSize)
  : m_cacheSize(cacheSize)
{
}

MwmSet::~MwmSet()
{
  // Need do call Cleanup() in derived class.
  ASSERT ( m_cache.empty(), () );
}

void MwmSet::Cleanup()
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  ClearCacheImpl(m_cache.begin(), m_cache.end());

#ifdef DEBUG
  for (MwmId i = 0; i < m_info.size(); ++i)
  {
    if (m_info[i].IsActive())
    {
      ASSERT_EQUAL(m_info[i].m_lockCount, 0, (i, m_name[i]));
      ASSERT_NOT_EQUAL(m_name[i], string(), (i));
    }
  }
#endif
}

void MwmSet::UpdateMwmInfo(MwmId id)
{
  if (m_info[id].m_status == MwmInfo::STATUS_TO_REMOVE)
    (void)RemoveImpl(id);
}

MwmSet::MwmId MwmSet::GetFreeId()
{
  MwmId const size = m_info.size();
  for (MwmId i = 0; i < size; ++i)
  {
    if (m_info[i].m_status == MwmInfo::STATUS_REMOVED)
      return i;
  }

  m_info.push_back(MwmInfo());
  m_name.push_back(string());
  return size;
}

MwmSet::MwmId MwmSet::GetIdByName(string const & name)
{
  ASSERT ( !name.empty(), () );

  for (MwmId i = 0; i < m_info.size(); ++i)
  {
    UpdateMwmInfo(i);

    if (m_name[i] == name)
    {
      ASSERT_NOT_EQUAL ( m_info[i].m_status, MwmInfo::STATUS_REMOVED, () );
      return i;
    }
  }

  return INVALID_MWM_ID;
}

int MwmSet::Add(string const & fileName, m2::RectD & rect)
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  MwmId const id = GetIdByName(fileName);
  if (id != INVALID_MWM_ID)
  {
    if (m_info[id].IsExist())
      LOG(LWARNING, ("Trying to add already added map", fileName));
    else
      m_info[id].m_status = MwmInfo::STATUS_ACTIVE;

    return -1;
  }

  return AddImpl(fileName, rect);
}

int MwmSet::AddImpl(string const & fileName, m2::RectD & rect)
{
  // this function can throw an exception for bad mwm file
  MwmInfo info;
  int const version = GetInfo(fileName, info);
  if (version == -1)
    return -1;

  info.m_status = MwmInfo::STATUS_ACTIVE;

  MwmId const id = GetFreeId();
  m_name[id] = fileName;
  m_info[id] = info;

  rect = info.m_limitRect;
  ASSERT ( rect.IsValid(), () );
  return version;
}

bool MwmSet::RemoveImpl(MwmId id)
{
  if (m_info[id].m_lockCount == 0)
  {
    m_name[id].clear();
    m_info[id].m_status = MwmInfo::STATUS_REMOVED;
    return true;
  }
  else
  {
    m_info[id].m_status = MwmInfo::STATUS_TO_REMOVE;
    return false;
  }
}

void MwmSet::Remove(string const & fileName)
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  (void)RemoveImpl(fileName);
}

bool MwmSet::RemoveImpl(string const & fileName)
{
  bool ret = true;

  MwmId const id = GetIdByName(fileName);
  if (id != INVALID_MWM_ID)
  {
    ret = RemoveImpl(id);

    ClearCache(id);
  }

  return ret;
}

void MwmSet::RemoveAll()
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  for (MwmId i = 0; i < m_info.size(); ++i)
    (void)RemoveImpl(i);

  // do not call ClearCache - it's under mutex lock
  ClearCacheImpl(m_cache.begin(), m_cache.end());
}

bool MwmSet::IsLoaded(string const & file) const
{
  MwmSet * p = const_cast<MwmSet *>(this);

  threads::MutexGuard mutexGuard(p->m_lock);
  UNUSED_VALUE(mutexGuard);

  MwmId const id = p->GetIdByName(file + DATA_FILE_EXTENSION);
  return (id != INVALID_MWM_ID && m_info[id].IsExist());
}

void MwmSet::GetMwmInfo(vector<MwmInfo> & info) const
{
  MwmSet * p = const_cast<MwmSet *>(this);

  threads::MutexGuard mutexGuard(p->m_lock);
  UNUSED_VALUE(mutexGuard);

  for (MwmId i = 0; i < m_info.size(); ++i)
    p->UpdateMwmInfo(i);

  info = m_info;
}

MwmSet::MwmValueBase * MwmSet::LockValue(MwmId id)
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  ASSERT_LESS(id, m_info.size(), ());
  if (id >= m_info.size())
    return NULL;

  UpdateMwmInfo(id);
  if (!m_info[id].IsActive())
    return NULL;

  ++m_info[id].m_lockCount;

  // Search in cache.
  for (CacheType::iterator it = m_cache.begin(); it != m_cache.end(); ++it)
  {
    if (it->first == id)
    {
      MwmValueBase * result = it->second;
      m_cache.erase(it);
      return result;
    }
  }
  return CreateValue(m_name[id]);
}

void MwmSet::UnlockValue(MwmId id, MwmValueBase * p)
{
  threads::MutexGuard mutexGuard(m_lock);
  UNUSED_VALUE(mutexGuard);

  ASSERT(p, (id));
  ASSERT_LESS(id, m_info.size(), ());
  if (id >= m_info.size() || p == 0)
    return;

  ASSERT_GREATER(m_info[id].m_lockCount, 0, ());
  if (m_info[id].m_lockCount > 0)
    --m_info[id].m_lockCount;
  UpdateMwmInfo(id);

  if (m_info[id].IsActive())
  {
    m_cache.push_back(make_pair(id, p));
    if (m_cache.size() > m_cacheSize)
    {
      ASSERT_EQUAL(m_cache.size(), m_cacheSize + 1, ());
      delete m_cache.front().second;
      m_cache.pop_front();
    }
  }
  else
    delete p;
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

namespace
{
  struct MwmIdIsEqualTo
  {
    MwmSet::MwmId m_id;
    explicit MwmIdIsEqualTo(MwmSet::MwmId id) : m_id(id) {}
    bool operator() (pair<MwmSet::MwmId, MwmSet::MwmValueBase *> const & p) const
    {
      return (p.first == m_id);
    }
  };
}

void MwmSet::ClearCache(MwmId id)
{
  ClearCacheImpl(RemoveIfKeepValid(m_cache.begin(), m_cache.end(), MwmIdIsEqualTo(id)), m_cache.end());
}
