#pragma once
#include "../geometry/rect2d.hpp"

#include "../std/deque.hpp"
#include "../std/mutex.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"

/// Information about stored mwm.
class MwmInfo
{
public:
  enum MwmTypeT
  {
    COUNTRY,
    WORLD,
    COASTS
  };

  enum Status
  {
    STATUS_UP_TO_DATE,            ///< Mwm is registered and up-to-date
    STATUS_MARKED_TO_DEREGISTER,  ///< Mwm is marked to be deregistered as soon as possible
    STATUS_DEREGISTERED,          ///< Mwm is deregistered
    STATUS_PENDING_UPDATE         ///< Mwm is registered but there're a pending update to it
  };

  MwmInfo();

  m2::RectD m_limitRect;  ///< Limit rect of mwm.
  uint8_t m_minScale;     ///< Min zoom level of mwm.
  uint8_t m_maxScale;     ///< Max zoom level of mwm.

  inline bool IsRegistered() const
  {
    return m_status == STATUS_UP_TO_DATE || m_status == STATUS_PENDING_UPDATE;
  }

  inline bool IsUpToDate() const { return m_status == STATUS_UP_TO_DATE; }

  MwmTypeT GetType() const;

  inline void SetStatus(Status status) { m_status = status; }

  inline Status GetStatus() const { return m_status; }

  uint8_t m_lockCount;  ///< Number of locks.

private:
  Status m_status;  ///< Current country status.
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

  /// Registers new map in the set.
  /// @param[in]  fileName  File name (without full path) of country.
  /// @param[out] rect      Limit rect of country.
  /// @return Map format version or -1 if not added (already exists).
  //@{
protected:
  int RegisterImpl(string const & fileName, m2::RectD & rect);

public:
  int Register(string const & fileName, m2::RectD & rect);
  //@}

  /// Used in unit tests only.
  inline void Register(string const & fileName)
  {
    m2::RectD dummy;
    CHECK(Register(fileName, dummy), ());
  }

  /// @name Remove mwm.
  //@{
protected:
  /// Deregisters map from the set when it' possible. Note that
  /// underlying file is not deleted.
  /// @return true - map is free; false - map is busy
  //@{
  bool DeregisterImpl(MwmId id);
  bool DeregisterImpl(string const & fileName);
  //@}

public:
  void Deregister(string const & fileName);
  void DeregisterAll();
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

  mutex m_lock;
};
