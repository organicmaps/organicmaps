#pragma once

#include "../drape/drape_global.hpp"

#include "../geometry/point2d.hpp"

#include "../base/mutex.hpp"

#include "../std/atomic.hpp"

namespace df
{

class UserPointMark
{
public:
  virtual ~UserPointMark() {}
  virtual m2::PointD const & GetPivot() const = 0;
  virtual string GetSymbolName() const  = 0;
  virtual dp::Anchor GetAnchor() const = 0;
  virtual float GetDepth() const = 0;
};

class UserLineMark
{
public:
  virtual ~UserLineMark() {}

  virtual size_t GetLayerCount() const = 0;
  virtual dp::Color const & GetColor(size_t layerIndex) const = 0;
  virtual float GetWidth(size_t layerIndex) const = 0;
  virtual float GetLayerDepth(size_t layerIndex) const = 0;

  /// Line geometry enumeration
  virtual size_t GetPointCount() const = 0;
  virtual m2::PointD const & GetPoint(size_t pointIndex) const = 0;
};

class UserMarksProvider
{
public:
  UserMarksProvider();
  virtual ~UserMarksProvider() {}

  void BeginRead();
    bool IsDirty() const;
    virtual bool IsDrawable() const = 0;

    virtual size_t GetUserPointCount() const = 0;
    /// never store UserPointMark reference
    virtual UserPointMark const * GetUserPointMark(size_t index) const = 0;

    virtual size_t GetUserLineCount() const = 0;
    /// never store UserLineMark reference
    virtual UserLineMark const * GetUserLineMark(size_t index) const = 0;
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
