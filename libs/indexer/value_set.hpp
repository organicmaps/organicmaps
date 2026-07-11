#pragma once

#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"

#include <algorithm>
#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

// The generic machinery of a registry of file-backed values (see MwmSet and terrain::TwmSet):
// refcounted infos, RAII handles, a size-capped cache of the opened values with the exclusive
// handoff (a cached value is popped for a single locker; concurrent lockers get own values,
// so the values need no internal thread safety), and the delayed deregistration (a file
// locked by outstanding handles is marked and actually deregistered by the last unlock).
namespace ds
{
// The status and the refcount of a registered file, base for the registry infos.
class SetInfoBase
{
public:
  enum Status
  {
    STATUS_REGISTERED,            ///< The file is registered and up to date.
    STATUS_MARKED_TO_DEREGISTER,  ///< The file is marked to be deregistered as soon as possible.
    STATUS_DEREGISTERED,          ///< The file is deregistered.
  };

  virtual ~SetInfoBase() = default;

  Status GetStatus() const { return m_status; }
  bool IsUpToDate() const { return IsRegistered(); }
  bool IsRegistered() const { return m_status == STATUS_REGISTERED; }

protected:
  Status SetStatus(Status status)
  {
    Status result = m_status;
    m_status = status;
    return result;
  }

  /// Writers are serialized by the owning set's mutex; the atomic makes the lock-free
  /// reads of SetId::IsAlive safe.
  std::atomic<Status> m_status{STATUS_DEREGISTERED};
  uint32_t m_numRefs = 0;  ///< Number of active handles.

  template <typename, typename, typename>
  friend class ValueSetBase;
};

// The id of a registered file: a shared info wrapper compared by the pointer identity.
template <typename InfoT>
class SetId
{
public:
  using InfoType = InfoT;

  SetId() = default;
  explicit SetId(std::shared_ptr<InfoT> const & info) : m_info(info) {}

  void Reset() { m_info.reset(); }
  bool IsNull() const { return m_info == nullptr; }
  bool IsAlive() const { return m_info && m_info->GetStatus() != SetInfoBase::STATUS_DEREGISTERED; }

  std::shared_ptr<InfoT> const & GetInfo() const { return m_info; }

  bool operator==(SetId const & rhs) const { return GetInfo() == rhs.GetInfo(); }
  bool operator!=(SetId const & rhs) const { return !(*this == rhs); }
  bool operator<(SetId const & rhs) const { return GetInfo() < rhs.GetInfo(); }

protected:
  std::shared_ptr<InfoT> m_info;
};

// The registry machinery. The derived set owns the actual registry (its keys and lookup)
// and the typed events/observers; EventsT is the derived event list (default-constructible,
// filled under the lock, processed by ProcessEvents strictly outside of it).
template <typename IdT, typename ValueT, typename EventsT>
class ValueSetBase
{
public:
  using InfoT = typename IdT::InfoType;

  explicit ValueSetBase(size_t cacheSize) : m_cacheSize(cacheSize) {}
  virtual ~ValueSetBase() = default;

  // A handle, which is used to refer to a value and prevent the file from deletion
  // while the value is used.
  class Handle
  {
  public:
    Handle() : m_set(nullptr), m_value(nullptr) {}

    Handle(Handle && handle) : m_id(std::move(handle.m_id)), m_set(handle.m_set), m_value(std::move(handle.m_value))
    {
      handle.m_set = nullptr;
      handle.m_id.Reset();
      handle.m_value = nullptr;
    }

    ~Handle()
    {
      if (m_set && m_value)
        m_set->UnlockValue(m_id, std::move(m_value));
    }

    // Returns a non-owning ptr.
    ValueT * GetValue() const { return m_value.get(); }

    bool IsAlive() const { return m_value.get() != nullptr; }
    IdT const & GetId() const { return m_id; }

    std::shared_ptr<InfoT> const & GetInfo() const
    {
      ASSERT(IsAlive(), ("The handle is not active."));
      return m_id.GetInfo();
    }

    Handle & operator=(Handle && handle)
    {
      std::swap(m_set, handle.m_set);
      std::swap(m_id, handle.m_id);
      std::swap(m_value, handle.m_value);
      return *this;
    }

  private:
    friend class ValueSetBase;

    Handle(ValueSetBase & set, IdT const & id, std::unique_ptr<ValueT> && value)
      : m_id(id)
      , m_set(&set)
      , m_value(std::move(value))
    {}

    IdT m_id;
    ValueSetBase * m_set;
    std::unique_ptr<ValueT> m_value;

    DISALLOW_COPY(Handle);
  };

  void ClearCache()
  {
    std::lock_guard<std::mutex> lock(m_lock);
    ClearCacheImpl();
  }

  // Clears the values cache and the registry. The infos are NOT marked as deregistered
  // (the outstanding ids and handles stay alive), no events are sent.
  void Clear()
  {
    std::lock_guard<std::mutex> lock(m_lock);
    ClearCacheImpl();
    m_registry.clear();
    OnClear();
  }

  /// All the actual (the latest per key, including the marked to deregister) infos.
  void GetInfos(std::vector<std::shared_ptr<InfoT>> & infos) const
  {
    std::lock_guard<std::mutex> lock(m_lock);
    infos.clear();
    infos.reserve(m_registry.size());
    for (auto const & [key, keyInfos] : m_registry)
      if (!keyInfos.empty())
        infos.push_back(keyInfos.back());
  }

protected:
  /// The registry key of the file (the country name, the file path, ...).
  virtual std::string const & GetRegistryKey(InfoT const & info) const = 0;

  /// Opens the value of the file. Called under m_lock; may throw for a bad file.
  virtual std::unique_ptr<ValueT> CreateValue(InfoT & info) const = 0;

  /// Sets the status and maps the transition to the derived typed events.
  /// @precondition Always called under m_lock.
  virtual void SetStatus(InfoT & info, SetInfoBase::Status status, EventsT & events) = 0;

  /// Notifies the derived observers. Always called OUTSIDE of m_lock: it's generally
  /// unsafe to call user-provided functions under the lock (deadlocks or unbounded waits).
  virtual void ProcessEvents(EventsT & events) = 0;

  /// A hook for the derived state reset (e.g. the condemned files).
  /// @precondition Always called under m_lock.
  virtual void OnClear() {}

  // This is the only valid way to take m_lock and use the *Impl() functions: the events
  // are collected under the lock and sent to the observers after it is released.
  template <typename TFn>
  void WithEventLog(TFn && fn)
  {
    EventsT events;
    {
      std::lock_guard<std::mutex> lock(m_lock);
      fn(events);
    }
    ProcessEvents(events);
  }

  /// Adds the info into the registry under its key (an old marked info may coexist there).
  /// @precondition Always called under m_lock.
  void AddToRegistryImpl(std::shared_ptr<InfoT> const & info) { m_registry[GetRegistryKey(*info)].push_back(info); }

  /// The id of the latest registration under the key or a null id.
  /// @precondition Always called under m_lock.
  IdT GetIdByKeyImpl(std::string const & key) const
  {
    auto const it = m_registry.find(key);
    if (it == m_registry.cend() || it->second.empty())
      return IdT();
    return IdT(it->second.back());
  }

  /// Deregisters the file or, when it is locked by outstanding handles, marks it to be
  /// deregistered by the last unlock. Removes at most one cached value.
  /// @return true if the file was deregistered right away.
  /// @precondition Always called under m_lock.
  bool DeregisterImpl(IdT const & id, EventsT & events)
  {
    if (!id.IsAlive())
      return false;

    std::shared_ptr<InfoT> const & info = id.GetInfo();
    if (info->m_numRefs == 0)
    {
      SetStatus(*info, SetInfoBase::STATUS_DEREGISTERED, events);
      auto & infos = m_registry[GetRegistryKey(*info)];
      infos.erase(std::remove(infos.begin(), infos.end(), info), infos.end());
      for (auto it = m_cache.begin(); it != m_cache.end(); ++it)
      {
        if (it->first == id)
        {
          m_cache.erase(it);
          break;
        }
      }
      return true;
    }

    SetStatus(*info, SetInfoBase::STATUS_MARKED_TO_DEREGISTER, events);
    return false;
  }

  std::unique_ptr<ValueT> LockValue(IdT const & id)
  {
    std::unique_ptr<ValueT> result;
    WithEventLog([&](EventsT & events) { result = LockValueImpl(id, events); });
    return result;
  }

  /// @precondition Always called under m_lock.
  std::unique_ptr<ValueT> LockValueImpl(IdT const & id, EventsT & events)
  {
    if (!id.IsAlive())
      return nullptr;
    std::shared_ptr<InfoT> info = id.GetInfo();

    // It's better to return a valid value even for the "out-of-date" (marked) files,
    // because they can be locked for a long time by other algos.

    ++info->m_numRefs;

    // Search in the cache: a hit pops the value out, so it is used exclusively.
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it)
    {
      if (it->first == id)
      {
        std::unique_ptr<ValueT> result = std::move(it->second);
        m_cache.erase(it);
        return result;
      }
    }

    try
    {
      return CreateValue(*info);
    }
    catch (Reader::TooManyFilesException const & ex)
    {
      LOG(LERROR, ("Too many open files, can't open:", id));
      --info->m_numRefs;
      return nullptr;
    }
    catch (std::exception const & ex)
    {
      LOG(LERROR, ("Can't create the value for", id, "Reason", ex.what()));
      --info->m_numRefs;
      DeregisterImpl(id, events);
      return nullptr;
    }
  }

  void UnlockValue(IdT const & id, std::unique_ptr<ValueT> p)
  {
    WithEventLog([&](EventsT & events) { UnlockValueImpl(id, std::move(p), events); });
  }

  /// @precondition Always called under m_lock.
  void UnlockValueImpl(IdT const & id, std::unique_ptr<ValueT> p, EventsT & events)
  {
    ASSERT(id.IsAlive(), (id));
    ASSERT(p.get() != nullptr, ());
    if (!id.IsAlive() || !p)
      return;

    std::shared_ptr<InfoT> const & info = id.GetInfo();
    ASSERT_GREATER(info->m_numRefs, 0, ());
    --info->m_numRefs;
    if (info->m_numRefs == 0 && info->GetStatus() == SetInfoBase::STATUS_MARKED_TO_DEREGISTER)
      VERIFY(DeregisterImpl(id, events), ());

    if (info->IsUpToDate())
    {
      /// @todo Probably, it's better to store only "unique by id" free caches here.
      /// But it's no obvious if we have many threads working with the single file.

      m_cache.push_back(std::make_pair(id, std::move(p)));
      if (m_cache.size() > m_cacheSize)
      {
        LOG(LDEBUG, ("The value cache size is reached! Added", id, "removed", m_cache.front().first));
        ASSERT_EQUAL(m_cache.size(), m_cacheSize + 1, ());
        m_cache.pop_front();
      }
    }
  }

  /// @precondition Always called under m_lock.
  Handle GetHandleByIdImpl(IdT const & id, EventsT & events)
  {
    std::unique_ptr<ValueT> value;
    if (id.IsAlive())
      value = LockValueImpl(id, events);
    return Handle(*this, id, std::move(value));
  }

  /// Clears the whole values cache.
  /// @precondition Always called under m_lock.
  void ClearCacheImpl() { m_cache.clear(); }

  /// Clears all the cached values of the id.
  /// @precondition Always called under m_lock.
  void ClearCache(IdT const & id)
  {
    std::erase_if(m_cache, [&id](auto const & p) { return p.first == id; });
  }

  mutable std::mutex m_lock;

  /// Registered infos by their key; a vector per key, because during a replacement the
  /// old marked-to-deregister info coexists with the new one (the back is the actual).
  std::map<std::string, std::vector<std::shared_ptr<InfoT>>> m_registry;

private:
  using CacheT = std::deque<std::pair<IdT, std::unique_ptr<ValueT>>>;

  CacheT m_cache;
  size_t const m_cacheSize;
};
}  // namespace ds
