#pragma once

#include "indexer/mwm_version.hpp"

#include "geometry/rect2d.hpp"

#include "base/macros.hpp"

#include "std/deque.hpp"
#include "std/iostream.hpp"
#include "std/map.hpp"
#include "std/mutex.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

/// Information about stored mwm.
class MwmInfo
{
public:
  friend class MwmSet;
  friend class Index;

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
  version::MwmVersion m_version;  ///< Mwm file version.

  inline Status GetStatus() const { return m_status; }

  inline bool IsRegistered() const
  {
    return m_status == STATUS_UP_TO_DATE || m_status == STATUS_PENDING_UPDATE;
  }

  inline bool IsUpToDate() const { return m_status == STATUS_UP_TO_DATE; }

  inline string const & GetFileName() const { return m_fileName; }

  MwmTypeT GetType() const;

private:
  inline void SetStatus(Status status) { m_status = status; }

  string m_fileName;    ///< Path to the mwm file.
  Status m_status;      ///< Current country status.
  uint8_t m_lockCount;  ///< Number of locks.
};

class MwmSet
{
public:
  using TMwmFileName = string;
  using TMwmInfoTable = map<TMwmFileName, shared_ptr<MwmInfo>>;

  struct MwmId
  {
  public:
    friend class MwmSet;

    MwmId() = default;
    MwmId(MwmId const & id) = default;
    MwmId(shared_ptr<MwmInfo> const & info) : m_info(info) {}

    void Reset() { m_info.reset(); }
    bool IsAlive() const
    {
      return m_info.get() != nullptr && m_info->GetStatus() != MwmInfo::STATUS_DEREGISTERED;
    }
    shared_ptr<MwmInfo> const & GetInfo() const { return m_info; }

    inline bool operator==(MwmId const & rhs) const { return GetInfo() == rhs.GetInfo(); }
    inline bool operator!=(MwmId const & rhs) const { return !(*this == rhs); }
    inline bool operator<(MwmId const & rhs) const { return GetInfo() < rhs.GetInfo(); }

    friend ostream & operator<<(ostream & os, MwmId const & id)
    {
      if (id.m_info.get())
        os << "MwmId[" << id.m_info->GetFileName() << "]";
      else
        os << "MwmId[invalid]";
      return os;
    }

  private:
    shared_ptr<MwmInfo> m_info;
  };

public:
  explicit MwmSet(size_t cacheSize = 5);
  virtual ~MwmSet() = 0;

  class MwmValueBase
  {
  public:
    virtual ~MwmValueBase() {}
  };

  using TMwmValueBasePtr = shared_ptr<MwmValueBase>;

  // Mwm lock, which is used to lock mwm when its FileContainer is used.
  class MwmLock final
  {
  public:
    MwmLock();
    MwmLock(MwmSet & mwmSet, MwmId const & mwmId);
    MwmLock(MwmLock && lock);
    ~MwmLock();

    // Returns a non-owning ptr.
    template <typename T>
    inline T * GetValue() const
    {
      return static_cast<T *>(m_value.get());
    }

    inline bool IsLocked() const { return m_value.get() != nullptr; }
    inline MwmId const & GetId() const { return m_mwmId; }
    shared_ptr<MwmInfo> const & GetInfo() const;

    MwmLock & operator=(MwmLock && lock);

  private:
    friend class MwmSet;

    MwmLock(MwmSet & mwmSet, MwmId const & mwmId, TMwmValueBasePtr value);

    MwmSet * m_mwmSet;
    MwmId m_mwmId;
    TMwmValueBasePtr m_value;

    DISALLOW_COPY(MwmLock);
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
  WARN_UNUSED_RESULT pair<MwmLock, bool> RegisterImpl(TMwmFileName const & fileName);

public:
  WARN_UNUSED_RESULT pair<MwmLock, bool> Register(TMwmFileName const & fileName);
  //@}

  /// @name Remove mwm.
  //@{
protected:
  /// Deregisters a map from the set when it's possible. Note that an
  /// underlying file is not deleted.
  ///
  /// @return true when the map was deregistered.
  //@{
  bool DeregisterImpl(MwmId const & id);
  bool DeregisterImpl(TMwmFileName const & ofileName);
  //@}

public:
  bool Deregister(TMwmFileName const & fileName);
  void DeregisterAll();
  //@}

  /// @param[in] file File name without extension.
  bool IsLoaded(TMwmFileName const & fileName) const;

  /// Get ids of all mwms. Some of them may be with not active status.
  /// In that case, LockValue returns NULL.
  void GetMwmsInfo(vector<shared_ptr<MwmInfo>> & info) const;

  void ClearCache();

  MwmId GetMwmIdByFileName(TMwmFileName const & fileName) const;

  MwmLock GetMwmLockByFileName(TMwmFileName const & fileName);

protected:
  /// @return True when file format version was successfully read to MwmInfo.
  virtual bool GetVersion(TMwmFileName const & fileName, MwmInfo & info) const = 0;
  virtual TMwmValueBasePtr CreateValue(string const & name) const = 0;

  void Cleanup();

private:
  typedef deque<pair<MwmId, TMwmValueBasePtr>> CacheType;

  TMwmValueBasePtr LockValue(MwmId const & id);
  TMwmValueBasePtr LockValueImpl(MwmId const & id);
  void UnlockValue(MwmId const & id, TMwmValueBasePtr p);
  void UnlockValueImpl(MwmId const & id, TMwmValueBasePtr p);

  /// Do the cleaning for [beg, end) without acquiring the mutex.
  /// @precondition This function is always called under mutex m_lock.
  void ClearCacheImpl(CacheType::iterator beg, CacheType::iterator end);

  CacheType m_cache;
  size_t const m_cacheSize;

protected:
  /// @precondition This function is always called under mutex m_lock.
  void ClearCache(MwmId const & id);

  /// Find mwm with a given name.
  /// @precondition This function is always called under mutex m_lock.
  MwmId GetMwmIdByFileNameImpl(TMwmFileName const & fileName) const;

  /// @precondition This function is always called under mutex m_lock.
  WARN_UNUSED_RESULT inline MwmLock GetLock(MwmId const & id)
  {
    return MwmLock(*this, id, LockValueImpl(id));
  }

  // This method is called under m_lock when mwm is removed from a
  // registry.
  virtual void OnMwmDeleted(shared_ptr<MwmInfo> const & info) {}

  // This method is called under m_lock when mwm is ready for update.
  virtual void OnMwmReadyForUpdate(shared_ptr<MwmInfo> const & info) {}

  TMwmInfoTable m_info;

  mutable mutex m_lock;
};
