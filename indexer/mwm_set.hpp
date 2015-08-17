#pragma once

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/mwm_version.hpp"

#include "geometry/rect2d.hpp"

#include "base/macros.hpp"

#include "std/deque.hpp"
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
    STATUS_REGISTERED,            ///< Mwm is registered and up to date.
    STATUS_MARKED_TO_DEREGISTER,  ///< Mwm is marked to be deregistered as soon as possible.
    STATUS_DEREGISTERED,          ///< Mwm is deregistered.
  };

  MwmInfo();
  virtual ~MwmInfo() = default;

  m2::RectD m_limitRect;          ///< Limit rect of mwm.
  uint8_t m_minScale;             ///< Min zoom level of mwm.
  uint8_t m_maxScale;             ///< Max zoom level of mwm.
  version::MwmVersion m_version;  ///< Mwm file version.

  inline Status GetStatus() const { return m_status; }

  inline bool IsUpToDate() const { return IsRegistered(); }

  inline bool IsRegistered() const
  {
    return m_status == STATUS_REGISTERED;
  }

  inline platform::LocalCountryFile const & GetLocalFile() const { return m_file; }

  inline string const & GetCountryName() const { return m_file.GetCountryName(); }

  inline int64_t GetVersion() const { return m_file.GetVersion(); }

  MwmTypeT GetType() const;

  /// Returns the lock counter value for test needs.
  uint8_t GetNumRefs() const { return m_numRefs; }

private:
  inline void SetStatus(Status status) { m_status = status; }

  platform::LocalCountryFile m_file;  ///< Path to the mwm file.
  Status m_status;                    ///< Current country status.
  uint8_t m_numRefs;                  ///< Number of active handles.
};

class MwmSet
{
public:
  class MwmId
  {
  public:
    friend class MwmSet;

    MwmId() = default;
    MwmId(shared_ptr<MwmInfo> const & info) : m_info(info) {}

    void Reset() { m_info.reset(); }
    bool IsAlive() const
    {
      return (m_info && m_info->GetStatus() != MwmInfo::STATUS_DEREGISTERED);
    }
    shared_ptr<MwmInfo> const & GetInfo() const { return m_info; }

    inline bool operator==(MwmId const & rhs) const { return GetInfo() == rhs.GetInfo(); }
    inline bool operator!=(MwmId const & rhs) const { return !(*this == rhs); }
    inline bool operator<(MwmId const & rhs) const { return GetInfo() < rhs.GetInfo(); }

    friend string DebugPrint(MwmId const & id);

  private:
    shared_ptr<MwmInfo> m_info;
  };

public:
  explicit MwmSet(size_t cacheSize = 5) : m_cacheSize(cacheSize) {}
  virtual ~MwmSet() = default;

  class MwmValueBase
  {
  public:
    virtual ~MwmValueBase() = default;
  };

  using TMwmValuePtr = MwmValueBase *;

  // Mwm handle, which is used to refer to mwm and prevent it from
  // deletion when its FileContainer is used.
  class MwmHandle final
  {
  public:
    MwmHandle();
    MwmHandle(MwmHandle && handle);
    ~MwmHandle();

    // Returns a non-owning ptr.
    template <typename T>
    inline T * GetValue() const
    {
      return static_cast<T *>(m_value);
    }

    inline bool IsAlive() const { return m_value; }
    inline MwmId const & GetId() const { return m_mwmId; }
    shared_ptr<MwmInfo> const & GetInfo() const;

    MwmHandle & operator=(MwmHandle && handle);

  private:
    friend class MwmSet;

    MwmHandle(MwmSet & mwmSet, MwmId const & mwmId, TMwmValuePtr value);

    MwmSet * m_mwmSet;
    MwmId m_mwmId;
    TMwmValuePtr m_value;

    DISALLOW_COPY(MwmHandle);
  };

  enum class RegResult
  {
    Success,
    VersionAlreadyExists,
    VersionTooOld,
    UnsupportedFileFormat,
    BadFile
  };

  /// Registers a new map.
  ///
  /// \return An active mwm handle when an mwm file with this version
  /// already exists (in this case mwm handle will point to already
  /// registered file) or when all registered corresponding mwm files
  /// are older than the localFile (in this case mwm handle will point
  /// to just-registered file).
protected:
  WARN_UNUSED_RESULT pair<MwmHandle, RegResult> RegisterImpl(
      platform::LocalCountryFile const & localFile);

public:
  WARN_UNUSED_RESULT pair<MwmHandle, RegResult> Register(
      platform::LocalCountryFile const & localFile);
  //@}

  /// @name Remove mwm.
  //@{
protected:
  /// Deregisters a map from internal records.
  ///
  /// \param countryFile A countryFile denoting a map to be deregistered.
  /// \return True if the map was successfully deregistered. If map is locked
  ///         now, returns false.
  //@{
  bool DeregisterImpl(MwmId const & id);
  bool DeregisterImpl(platform::CountryFile const & countryFile);
  //@}

public:
  bool Deregister(platform::CountryFile const & countryFile);
  //@}

  /// Returns true when country is registered and can be used.
  bool IsLoaded(platform::CountryFile const & countryFile) const;

  /// Get ids of all mwms. Some of them may be with not active status.
  /// In that case, LockValue returns NULL.
  void GetMwmsInfo(vector<shared_ptr<MwmInfo>> & info) const;

  // Clears caches and mwm's registry. All known mwms won't be marked as DEREGISTERED.
  void Clear();

  void ClearCache();

  MwmId GetMwmIdByCountryFile(platform::CountryFile const & countryFile) const;

  MwmHandle GetMwmHandleByCountryFile(platform::CountryFile const & countryFile);

  MwmHandle GetMwmHandleById(MwmId const & id);

  /// Now this function looks like workaround, but it allows to avoid ugly const_cast everywhere..
  /// Client code usually holds const reference to Index, but implementation is non-const.
  /// @todo Actually, we need to define, is this behaviour (getting Handle) const or non-const.
  inline MwmHandle GetMwmHandleById(MwmId const & id) const
  {
    return const_cast<MwmSet *>(this)->GetMwmHandleById(id);
  }

protected:
  /// @return True when file format version was successfully read to MwmInfo.
  virtual MwmInfo * CreateInfo(platform::LocalCountryFile const & localFile) const = 0;
  virtual MwmValueBase * CreateValue(MwmInfo & info) const = 0;

private:
  typedef deque<pair<MwmId, TMwmValuePtr>> CacheType;

  /// @precondition This function is always called under mutex m_lock.
  MwmHandle GetMwmHandleByIdImpl(MwmId const & id);

  TMwmValuePtr LockValue(MwmId const & id);
  TMwmValuePtr LockValueImpl(MwmId const & id);
  void UnlockValue(MwmId const & id, TMwmValuePtr p);
  void UnlockValueImpl(MwmId const & id, TMwmValuePtr p);

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
  MwmId GetMwmIdByCountryFileImpl(platform::CountryFile const & countryFile) const;

  /// @precondition This function is always called under mutex m_lock.
  WARN_UNUSED_RESULT inline MwmHandle GetLock(MwmId const & id)
  {
    return MwmHandle(*this, id, LockValueImpl(id));
  }

  // This method is called under m_lock when mwm is removed from a
  // registry.
  virtual void OnMwmDeregistered(platform::LocalCountryFile const & localFile) {}

  map<string, vector<shared_ptr<MwmInfo>>> m_info;

  mutable mutex m_lock;
};

string DebugPrint(MwmSet::RegResult result);
