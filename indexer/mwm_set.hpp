#pragma once

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/mwm_version.hpp"

#include "geometry/rect2d.hpp"

#include "base/macros.hpp"

#include "indexer/feature_meta.hpp"

#include "std/atomic.hpp"
#include "std/deque.hpp"
#include "std/map.hpp"
#include "std/mutex.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

#include "base/observer_list.hpp"

/// Information about stored mwm.
class MwmInfo
{
public:
  friend class Index;
  friend class MwmSet;

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

  inline feature::RegionData const & GetRegionData() const { return m_data; }

  /// Returns the lock counter value for test needs.
  uint8_t GetNumRefs() const { return m_numRefs; }

protected:
  inline Status SetStatus(Status status)
  {
    Status result = m_status;
    m_status = status;
    return result;
  }

  feature::RegionData m_data;

  platform::LocalCountryFile m_file;  ///< Path to the mwm file.
  atomic<Status> m_status;            ///< Current country status.
  uint32_t m_numRefs;                 ///< Number of active handles.
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
    shared_ptr<MwmInfo> & GetInfo() { return m_info; }
    shared_ptr<MwmInfo> const & GetInfo() const { return m_info; }

    inline bool operator==(MwmId const & rhs) const { return GetInfo() == rhs.GetInfo(); }
    inline bool operator!=(MwmId const & rhs) const { return !(*this == rhs); }
    inline bool operator<(MwmId const & rhs) const { return GetInfo() < rhs.GetInfo(); }

    friend string DebugPrint(MwmId const & id);

  private:
    shared_ptr<MwmInfo> m_info;
  };

public:
  explicit MwmSet(size_t cacheSize = 64) : m_cacheSize(cacheSize) {}
  virtual ~MwmSet() = default;

  class MwmValueBase
  {
  public:
    virtual ~MwmValueBase() = default;
  };

  // Mwm handle, which is used to refer to mwm and prevent it from
  // deletion when its FileContainer is used.
  class MwmHandle
  {
  public:
    MwmHandle();
    MwmHandle(MwmHandle && handle);
    ~MwmHandle();

    // Returns a non-owning ptr.
    template <typename T>
    inline T * GetValue() const
    {
      return static_cast<T *>(m_value.get());
    }

    inline bool IsAlive() const { return m_value.get() != nullptr; }
    inline MwmId const & GetId() const { return m_mwmId; }
    shared_ptr<MwmInfo> const & GetInfo() const;

    MwmHandle & operator=(MwmHandle && handle);

  protected:
    MwmId m_mwmId;

  private:
    friend class MwmSet;

    MwmHandle(MwmSet & mwmSet, MwmId const & mwmId, unique_ptr<MwmValueBase> && value);

    MwmSet * m_mwmSet;
    unique_ptr<MwmValueBase> m_value;

    DISALLOW_COPY(MwmHandle);
  };

  struct Event
  {
    enum Type
    {
      TYPE_REGISTERED,
      TYPE_DEREGISTERED,
      TYPE_UPDATED,
    };

    Event() = default;
    Event(Type type, platform::LocalCountryFile const & file)
      : m_type(type), m_file(file)
    {
    }
    Event(Type type, platform::LocalCountryFile const & newFile,
          platform::LocalCountryFile const & oldFile)
      : m_type(type), m_file(newFile), m_oldFile(oldFile)
    {
    }

    inline bool operator==(Event const & rhs) const
    {
      return m_type == rhs.m_type && m_file == rhs.m_file && m_oldFile == rhs.m_oldFile;
    }

    inline bool operator!=(Event const & rhs) const { return !(*this == rhs); }

    Type m_type;
    platform::LocalCountryFile m_file;
    platform::LocalCountryFile m_oldFile;
  };

  class EventList
  {
  public:
    EventList() = default;

    inline void Add(Event const & event) { m_events.push_back(event); }

    inline void Append(EventList const & events)
    {
      m_events.insert(m_events.end(), events.m_events.begin(), events.m_events.end());
    }

    vector<Event> const & Get() const { return m_events; }

  private:
    vector<Event> m_events;

    DISALLOW_COPY_AND_MOVE(EventList);
  };

  enum class RegResult
  {
    Success,
    VersionAlreadyExists,
    VersionTooOld,
    UnsupportedFileFormat,
    BadFile
  };

  // An Observer interface to MwmSet. Note that these functions can
  // be called from *ANY* thread because most signals are sent when
  // some thread releases its MwmHandle, so overrides must be as fast
  // as possible and non-blocking when it's possible.
  class Observer
  {
  public:
    virtual ~Observer() = default;

    // Called when a map is registered for a first time and can be
    // used.
    virtual void OnMapRegistered(platform::LocalCountryFile const & /*localFile*/) {}

    // Called when a map is updated to a newer version. Feel free to
    // treat it as combined OnMapRegistered(newFile) +
    // OnMapDeregistered(oldFile).
    virtual void OnMapUpdated(platform::LocalCountryFile const & /*newFile*/,
                              platform::LocalCountryFile const & /*oldFile*/) {}

    // Called when a map is deregistered and can no longer be used.
    virtual void OnMapDeregistered(platform::LocalCountryFile const & /*localFile*/) {}
  };

  /// Registers a new map.
  ///
  /// \return An active mwm handle when an mwm file with this version
  /// already exists (in this case mwm handle will point to already
  /// registered file) or when all registered corresponding mwm files
  /// are older than the localFile (in this case mwm handle will point
  /// to just-registered file).
protected:
  pair<MwmId, RegResult> RegisterImpl(platform::LocalCountryFile const & localFile, EventList & events);

public:
  pair<MwmId, RegResult> Register(platform::LocalCountryFile const & localFile);
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
  bool DeregisterImpl(MwmId const & id, EventList & events);
  bool DeregisterImpl(platform::CountryFile const & countryFile, EventList & events);
  //@}

public:
  bool Deregister(platform::CountryFile const & countryFile);
  //@}

  inline bool AddObserver(Observer & observer) { return m_observers.Add(observer); }

  inline bool RemoveObserver(Observer const & observer) { return m_observers.Remove(observer); }

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
  virtual unique_ptr<MwmInfo> CreateInfo(platform::LocalCountryFile const & localFile) const = 0;
  virtual unique_ptr<MwmValueBase> CreateValue(MwmInfo & info) const = 0;

private:
  typedef deque<pair<MwmId, unique_ptr<MwmValueBase>>> CacheType;

  // This is the only valid way to take |m_lock| and use *Impl()
  // functions. The reason is that event processing requires
  // triggering of observers, but it's generally unsafe to call
  // user-provided functions while |m_lock| is taken, as it may lead
  // to deadlocks or locks without any time guarantees. Instead, a
  // list of Events is used to collect all events and then send them
  // to observers without |m_lock| protection.
  template <typename TFn>
  void WithEventLog(TFn && fn)
  {
    EventList events;
    {
      lock_guard<mutex> lock(m_lock);
      fn(events);
    }
    ProcessEventList(events);
  }

  // Sets |status| in |info|, adds corresponding event to |event|.
  void SetStatus(MwmInfo & info, MwmInfo::Status status, EventList & events);

  // Triggers observers on each event in |events|.
  void ProcessEventList(EventList & events);

  /// @precondition This function is always called under mutex m_lock.
  MwmHandle GetMwmHandleByIdImpl(MwmId const & id, EventList & events);

  unique_ptr<MwmValueBase> LockValue(MwmId const & id);
  unique_ptr<MwmValueBase> LockValueImpl(MwmId const & id, EventList & events);
  void UnlockValue(MwmId const & id, unique_ptr<MwmValueBase> p);
  void UnlockValueImpl(MwmId const & id, unique_ptr<MwmValueBase> p, EventList & events);

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

  map<string, vector<shared_ptr<MwmInfo>>> m_info;

  mutable mutex m_lock;

private:
  base::ObserverListSafe<Observer> m_observers;
};

string DebugPrint(MwmSet::RegResult result);
string DebugPrint(MwmSet::Event::Type type);
string DebugPrint(MwmSet::Event const & event);
