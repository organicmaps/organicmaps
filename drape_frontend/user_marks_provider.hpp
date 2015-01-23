#pragma once

#include "../drape/drape_global.hpp"

#include "../geometry/point2d.hpp"

#include "../base/mutex.hpp"

#include "../std/atomic.hpp"

namespace df
{

class UserMarksProvider
{
public:
  UserMarksProvider();
  virtual ~UserMarksProvider() {}

  void BeginRead();
    bool IsDirty() const;
    virtual bool IsDrawable() const = 0;
    virtual size_t GetCount() const = 0;
    virtual m2::PointD const & GetPivot(size_t index) const = 0;
    virtual string const & GetSymbolName(size_t index) const = 0;
    virtual dp::Anchor GetAnchor(size_t index) const = 0;
    virtual float GetDepth(size_t index) const = 0;
  void EndRead();

  void IncrementCounter();
  void DecrementCounter();
  bool CanBeDeleted();
  bool IsPendingOnDelete();
  void DeleteLater();

protected:
  void BeginWrite();
    void SetDirty();
  void EndWrite();

private:
  void Lock();
  void Unlock();

private:
  threads::Mutex m_mutex;
  bool m_isDirty = false;
  atomic<bool> m_pendingOnDelete;
  atomic<int> m_counter;
};

} // namespace df
