#pragma once
#include "../geometry/rect2d.hpp"

#include "../base/mutex.hpp"

#include "../std/deque.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"


/// Information about stored mwm.
class MwmInfo
{
public:
  MwmInfo();

  m2::RectD m_limitRect;    ///< Limit rect of mwm.
  uint8_t m_minScale;       ///< Min zoom level of mwm.
  uint8_t m_maxScale;       ///< Max zoom level of mwm.

  inline bool IsExist() const
  {
    return (m_status == STATUS_ACTIVE || m_status == STATUS_UPDATE);
  }
  inline bool IsActive() const { return (m_status == STATUS_ACTIVE); }

  enum MwmTypeT { COUNTRY, WORLD, COASTS };
  MwmTypeT GetType() const;

  enum Status
  {
    STATUS_ACTIVE,
    STATUS_TO_REMOVE,
    STATUS_REMOVED,
    STATUS_UPDATE
  };

  uint8_t m_lockCount;      ///< Number of locks.
  uint8_t m_status;         ///< Current country status.
};

class MwmSet
{
public:
  typedef size_t MwmId;

  explicit MwmSet(size_t cacheSize = 5);
  virtual ~MwmSet() = 0;

  class MwmValueBase
  {
  public:
    virtual ~MwmValueBase() {}
  };

  // Mwm lock, which is used to lock mwm when its FileContainer is used.
  class MwmLock
  {
  public:
    MwmLock(MwmSet & mwmSet, MwmId mwmId);
    ~MwmLock();

    inline MwmValueBase * GetValue() const { return m_pValue; }
    inline MwmId GetID() const { return m_id; }

  private:
    MwmSet & m_mwmSet;
    MwmId m_id;
    MwmValueBase * m_pValue;
  };

  /// Add new map.
  /// @param[in]  fileName  File name (without full path) of country.
  /// @param[out] rect      Limit rect of country.
  /// @return Map format version or -1 if not added (already exists).
  //@{
protected:
  int AddImpl(string const & fileName, m2::RectD & rect);

public:
  int Add(string const & fileName, m2::RectD & rect);
  //@}

  /// Used in unit tests only.
  inline void Add(string const & fileName)
  {
    m2::RectD dummy;
    CHECK(Add(fileName, dummy), ());
  }

  /// @name Remove mwm.
  //@{
protected:
  /// @return true - map is free; false - map is busy
  //@{
  bool RemoveImpl(MwmId id);
  bool RemoveImpl(string const & fileName);
  //@}

public:
  void Remove(string const & fileName);
  void RemoveAll();
  //@}

  /// @param[in] file File name without extension.
  bool IsLoaded(string const & file) const;

  /// Get ids of all mwms. Some of them may be with not active status.
  /// In that case, LockValue returns NULL.
  void GetMwmInfo(vector<MwmInfo> & info) const;

  // Clear caches.
  void ClearCache();

protected:
  /// @return mwm format version
  virtual int GetInfo(string const & name, MwmInfo & info) const = 0;
  virtual MwmValueBase * CreateValue(string const & name) const = 0;

  void Cleanup();

private:
  typedef deque<pair<MwmId, MwmValueBase *> > CacheType;

  MwmValueBase * LockValue(MwmId id);
  void UnlockValue(MwmId id, MwmValueBase * p);

  /// Find first removed mwm or add a new one.
  /// @precondition This function is always called under mutex m_lock.
  MwmId GetFreeId();

  /// Do the cleaning for [beg, end) without acquiring the mutex.
  /// @precondition This function is always called under mutex m_lock.
  void ClearCacheImpl(CacheType::iterator beg, CacheType::iterator end);

  CacheType m_cache;
  size_t m_cacheSize;

protected:
  static const MwmId INVALID_MWM_ID = static_cast<MwmId>(-1);

  /// Find mwm with a given name.
  /// @precondition This function is always called under mutex m_lock.
  MwmId GetIdByName(string const & name);

  /// @precondition This function is always called under mutex m_lock.
  void ClearCache(MwmId id);

  /// Update given MwmInfo.
  /// @precondition This function is always called under mutex m_lock.
  virtual void UpdateMwmInfo(MwmId id);

  vector<MwmInfo> m_info;
  vector<string> m_name;
  threads::Mutex m_lock;
};
