#pragma once
#include "../geometry/rect2d.hpp"

#include "../base/mutex.hpp"

#include "../std/deque.hpp"
#include "../std/function.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"


// Information about stored mwm.
struct MwmInfo
{
  m2::RectD m_limitRect;    // Limit rect of mwm.
  uint8_t m_minScale;       // Min zoom level of mwm.
  uint8_t m_maxScale;       // Max zoom level of mwm.

  // Does this MwmInfo represent a valid Mwm?
  bool isValid() const { return m_status == STATUS_ACTIVE; }
private:
  friend class MwmSet;

  enum Status { STATUS_ACTIVE = 0, STATUS_TO_REMOVE = 1, STATUS_REMOVED = 2 };
  uint8_t m_lockCount;      // Number of locks.
  uint8_t m_status;         //
};

class MwmValue;

class MwmSet
{
protected:
  virtual void GetInfo(string const & name, MwmInfo & info) const = 0;
  virtual MwmValue * CreateValue(string const & name) const = 0;
  virtual void DestroyValue(MwmValue * p) const = 0;

  void Cleanup();

public:
  typedef size_t MwmId;

  explicit MwmSet(size_t cacheSize = 5);
  virtual ~MwmSet() = 0;

  // Mwm lock, which is used to lock mwm when its FileContainer is used.
  class MwmLock
  {
  public:
    MwmLock(MwmSet const & mwmSet, MwmId mwmId);
    ~MwmLock();

    inline MwmValue * GetValue() const { return m_pValue; }

  private:
    MwmSet const & m_mwmSet;
    MwmId m_id;
    MwmValue * m_pValue;
  };

  // Add new mwm. Returns false, if mwm with given fileName already exists.
  bool Add(string const & fileName);

  // Remove mwm.
  void Remove(string const & fileName);

  // Get ids of all mwms. Some of them may be marked to remove.
  void GetMwmInfo(vector<MwmInfo> & info) const;

  // Clear caches.
  void ClearCache();

private:
  friend class MwmLock;

  static const MwmId INVALID_MWM_ID = static_cast<MwmId>(-1);

  typedef deque<pair<MwmId, MwmValue *> > CacheType;

  // Update given MwmInfo.
  inline static void UpdateMwmInfo(MwmInfo & info);

  MwmValue * LockValue(MwmId id) const;
  void UnlockValue(MwmId id, MwmValue * p) const;

  // Find first removed mwm or add a new one.
  MwmId GetFreeId();

  // Find mwm with a given name.
  MwmId GetIdByName(string const & name);

  // Do the cleaning for [beg, end) without acquiring the mutex.
  void ClearCacheImpl(CacheType::iterator beg, CacheType::iterator end);

  mutable vector<MwmInfo> m_info;
  mutable vector<string> m_name;
  mutable CacheType m_cache;
  size_t m_cacheSize;
  mutable threads::Mutex m_lock;
};
