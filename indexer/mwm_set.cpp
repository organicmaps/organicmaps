#include "mwm_set.hpp"
#include "scales.hpp"

#include "../defines.hpp"

#include "../base/logging.hpp"
#include "../base/macros.hpp"
#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"

MwmInfo::MwmInfo() : m_lockCount(0), m_status(STATUS_DEREGISTERED)
{
  // Important: STATUS_DEREGISTERED - is the default value.
  // Apply STATUS_UP_TO_DATE before adding to maps container.
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
  lock_guard<mutex> lock(m_lock);

  ClearCacheImpl(m_cache.begin(), m_cache.end());

#ifdef DEBUG
  for (MwmId i = 0; i < m_info.size(); ++i)
  {
    if (m_info[i].IsUpToDate())
    {
      ASSERT_EQUAL(m_info[i].m_lockCount, 0, (i, m_name[i]));
      ASSERT_NOT_EQUAL(m_name[i], string(), (i));
    }
  }
#endif
}

void MwmSet::UpdateMwmInfo(MwmId id)
{
  if (m_info[id].GetStatus() == MwmInfo::STATUS_MARKED_TO_DEREGISTER)
    (void)DeregisterImpl(id);
}

MwmSet::MwmId MwmSet::GetFreeId()
{
  MwmId const size = m_info.size();
  for (MwmId i = 0; i < size; ++i)
  {
    if (m_info[i].GetStatus() == MwmInfo::STATUS_DEREGISTERED)
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
      ASSERT_NOT_EQUAL(m_info[i].GetStatus(), MwmInfo::STATUS_DEREGISTERED, ());
      return i;
    }
  }

  return INVALID_MWM_ID;
}

int MwmSet::Register(string const & fileName, m2::RectD & rect)
{
  lock_guard<mutex> lock(m_lock);

  MwmId const id = GetIdByName(fileName);
  if (id != INVALID_MWM_ID)
  {
    if (m_info[id].IsRegistered())
      LOG(LWARNING, ("Trying to add already added map", fileName));
    else
      m_info[id].SetStatus(MwmInfo::STATUS_UP_TO_DATE);

    return -1;
  }

  return RegisterImpl(fileName, rect);
}

int MwmSet::RegisterImpl(string const & fileName, m2::RectD & rect)
{
  // this function can throw an exception for bad mwm file
  MwmInfo info;
  int const version = GetInfo(fileName, info);
  if (version == -1)
    return -1;

  info.SetStatus(MwmInfo::STATUS_UP_TO_DATE);

  MwmId const id = GetFreeId();
  m_name[id] = fileName;
  m_info[id] = info;

  rect = info.m_limitRect;
  ASSERT ( rect.IsValid(), () );
  return version;
}

bool MwmSet::DeregisterImpl(MwmId id)
{
  if (m_info[id].m_lockCount == 0)
  {
    m_name[id].clear();
    m_info[id].SetStatus(MwmInfo::STATUS_DEREGISTERED);
    return true;
  }
  else
  {
    m_info[id].SetStatus(MwmInfo::STATUS_MARKED_TO_DEREGISTER);
    return false;
  }
}

void MwmSet::Deregister(string const & fileName)
{
  lock_guard<mutex> lock(m_lock);
  (void)DeregisterImpl(fileName);
}

bool MwmSet::DeregisterImpl(string const & fileName)
{
  bool ret = true;

  MwmId const id = GetIdByName(fileName);
  if (id != INVALID_MWM_ID)
  {
    ret = DeregisterImpl(id);

    ClearCache(id);
  }

  return ret;
}

void MwmSet::DeregisterAll()
{
  lock_guard<mutex> lock(m_lock);

  for (MwmId i = 0; i < m_info.size(); ++i)
    (void)DeregisterImpl(i);

  // do not call ClearCache - it's under mutex lock
  ClearCacheImpl(m_cache.begin(), m_cache.end());
}

bool MwmSet::IsLoaded(string const & file) const
{
  MwmSet * p = const_cast<MwmSet *>(this);
  lock_guard<mutex> lock(p->m_lock);

  MwmId const id = p->GetIdByName(file + DATA_FILE_EXTENSION);
  return (id != INVALID_MWM_ID && m_info[id].IsRegistered());
}

void MwmSet::GetMwmInfo(vector<MwmInfo> & info) const
{
  MwmSet * p = const_cast<MwmSet *>(this);
  lock_guard<mutex> lock(p->m_lock);

  for (MwmId i = 0; i < m_info.size(); ++i)
    p->UpdateMwmInfo(i);

  info = m_info;
}

MwmSet::MwmValueBase * MwmSet::LockValue(MwmId id)
{
  lock_guard<mutex> lock(m_lock);

  ASSERT_LESS(id, m_info.size(), ());
  if (id >= m_info.size())
    return NULL;

  UpdateMwmInfo(id);
  if (!m_info[id].IsUpToDate())
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
  lock_guard<mutex> lock(m_lock);

  ASSERT(p, (id));
  ASSERT_LESS(id, m_info.size(), ());
  if (id >= m_info.size() || p == 0)
    return;

  ASSERT_GREATER(m_info[id].m_lockCount, 0, ());
  if (m_info[id].m_lockCount > 0)
    --m_info[id].m_lockCount;
  UpdateMwmInfo(id);

  if (m_info[id].IsUpToDate())
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
  lock_guard<mutex> lock(m_lock);

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
