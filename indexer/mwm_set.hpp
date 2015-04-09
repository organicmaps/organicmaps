#pragma once

#include "indexer/mwm_version.hpp"

#include "geometry/rect2d.hpp"

#include "base/macros.hpp"

#include "std/deque.hpp"
#include "std/mutex.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

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

  m2::RectD m_limitRect;          ///< Limit rect of mwm.
  uint8_t m_minScale;             ///< Min zoom level of mwm.
  uint8_t m_maxScale;             ///< Max zoom level of mwm.
  uint8_t m_lockCount;            ///< Number of locks.
  version::MwmVersion m_version;  ///< Mwm file version.

  inline bool IsRegistered() const
  {
    return m_status == STATUS_UP_TO_DATE || m_status == STATUS_PENDING_UPDATE;
  }

  inline bool IsUpToDate() const { return m_status == STATUS_UP_TO_DATE; }

  MwmTypeT GetType() const;

  inline void SetStatus(Status status) { m_status = status; }

  inline Status GetStatus() const { return m_status; }

private:
  Status m_status;  ///< Current country status.
};

class MwmSet
{
public:
  typedef size_t MwmId;

  static const MwmId INVALID_MWM_ID;

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
    MwmLock();
    MwmLock(MwmSet & mwmSet, MwmId mwmId);
    MwmLock(MwmSet & mwmSet, string const & fileName);
    MwmLock(MwmLock && lock);
    virtual ~MwmLock();

    template <typename T>
    inline T * GetValue() const
    {
      return static_cast<T *>(m_value);
    }
    inline bool IsLocked() const { return m_value; }
    inline MwmId GetId() const { return m_mwmId; }
    MwmInfo const & GetInfo() const;

    MwmLock & operator=(MwmLock && lock);

  private:
    friend class MwmSet;

    MwmLock(MwmSet & mwmSet, MwmId mwmId, MwmValueBase * value);

    MwmSet * m_mwmSet;
    MwmId m_mwmId;
    MwmValueBase * m_value;

    NONCOPYABLE(MwmLock);
  };

  /// Registers a new map.
  ///
  /// \return A pair of an MwmLock and a flag. MwmLock is locked iff the
  ///         map with fileName was created or already exists. Flag
  ///         is set when the map was registered for a first
  ///         time. Thus, there are three main cases:
  ///
  ///         * the map already exists - returns active lock and unset flag
  ///         * the map was already registered - returns active lock and set flag
  ///         * the map can't be registered - returns inactive lock and unset flag
  //@{
protected:
  WARN_UNUSED_RESULT pair<MwmLock, bool> RegisterImpl(string const & fileName);

public:
  WARN_UNUSED_RESULT pair<MwmLock, bool> Register(string const & fileName);
  //@}

  /// @name Remove mwm.
  //@{
protected:
  /// Deregisters a map from the set when it's possible. Note that an
  /// underlying file is not deleted.
  ///
  /// @return true when the map was deregistered.
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

  /// \return A reference to an MwmInfo corresponding to id.  Id must
  ///         be a valid Mwm id.
  MwmInfo const & GetMwmInfo(MwmId id) const;

  // Clear caches.
  void ClearCache();

protected:
  /// @return True when it's possible to get file format version - in
  ///         this case version is set to the file format version.
  virtual bool GetVersion(string const & name, MwmInfo & info) const = 0;
  virtual MwmValueBase * CreateValue(string const & name) const = 0;

  void Cleanup();

private:
  typedef deque<pair<MwmId, MwmValueBase *> > CacheType;

  MwmValueBase * LockValue(MwmId id);
  MwmValueBase * LockValueImpl(MwmId id);
  void UnlockValue(MwmId id, MwmValueBase * p);
  void UnlockValueImpl(MwmId id, MwmValueBase * p);

  /// Find first removed mwm or add a new one.
  /// @precondition This function is always called under mutex m_lock.
  MwmId GetFreeId();

  /// Do the cleaning for [beg, end) without acquiring the mutex.
  /// @precondition This function is always called under mutex m_lock.
  void ClearCacheImpl(CacheType::iterator beg, CacheType::iterator end);

  CacheType m_cache;
  size_t m_cacheSize;

protected:
  /// Find mwm with a given name.
  /// @precondition This function is always called under mutex m_lock.
  MwmId GetIdByName(string const & name);

  /// @precondition This function is always called under mutex m_lock.
  void ClearCache(MwmId id);

  /// @precondition This function is always called under mutex m_lock.
  WARN_UNUSED_RESULT inline MwmLock GetLock(MwmId id)
  {
    return MwmLock(*this, id, LockValueImpl(id));
  }

  /// Update given MwmInfo.
  /// @precondition This function is always called under mutex m_lock.
  virtual void UpdateMwmInfo(MwmId id);

  vector<MwmInfo> m_info;

  vector<string> m_name;

  mutex m_lock;
};
