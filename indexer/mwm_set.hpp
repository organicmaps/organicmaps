#include "../geometry/rect2d.hpp"
#include "../base/mutex.hpp"
#include "../std/deque.hpp"
#include "../std/function.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"

class FilesContainerR;

// Information about stored mwm.
struct MwmInfo
{
  m2::RectU32 m_limitRect;  // Limit rect of mwm.
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

class MwmSet
{
public:

  typedef size_t MwmId;

  explicit MwmSet(function<void (string const &, MwmInfo &)> const & fnGetMwmInfo,
                  function<FilesContainerR * (string const &)> const & fnCreateContainer,
                  size_t cacheSize = 5);
  ~MwmSet();

  // Mwm lock, which is used to lock mwm when its FileContainer is used.
  class MwmLock
  {
  public:
    MwmLock(MwmSet & mwmSet, MwmId mwmId);
    ~MwmLock();

    FilesContainerR * GetFileContainer() const;
  private:
    MwmSet & m_mwmSet;
    MwmId m_id;
    FilesContainerR * m_pFileContainer;
  };

  // Add new mwm. Returns false, if mwm with given fileName already exists.
  bool Add(string const & fileName);

  // Remove mwm.
  void Remove(string const & fileName);

  // Get ids of all mwms. Some of them may be marked to remove.
  void GetMwmInfo(vector<MwmInfo> & info);

  // Clear caches.
  void ClearCache();

private:
  friend class MwmLock;

  static const MwmId INVALID_MWM_ID = static_cast<MwmId>(-1);

  typedef deque<pair<MwmId, FilesContainerR *> > CacheType;

  // Update given MwmInfo.
  inline static void UpdateMwmInfo(MwmInfo & info);

  FilesContainerR * LockContainer(MwmId id);
  void UnlockContainer(MwmId id, FilesContainerR * pContainer);

  // Find first removed mwm or add a new one.
  MwmId GetFreeId();

  // Find mwm with a given name.
  MwmId GetIdByName(string const & name);

  // Do the cleaning for [beg, end) without acquiring the mutex.
  void ClearCacheImpl(CacheType::iterator beg, CacheType::iterator end);

  vector<MwmInfo> m_info;
  vector<string> m_name;
  CacheType m_cache;
  size_t m_cacheSize;
  function<void (string const &, MwmInfo &)> const m_fnGetMwmInfo;
  function<FilesContainerR * (string const &)> const m_fnCreateContainer;
  threads::Mutex m_lock;
};
