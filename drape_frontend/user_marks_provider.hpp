#pragma once

#include "drape/drape_global.hpp"

#include "geometry/polyline2d.hpp"

#include "base/mutex.hpp"

#include <atomic>
#include <vector>

namespace df
{

class UserPointMark
{
public:
  virtual ~UserPointMark() {}
  virtual m2::PointD const & GetPivot() const = 0;
  virtual m2::PointD GetPixelOffset() const = 0;
  virtual std::string GetSymbolName() const  = 0;
  virtual dp::Anchor GetAnchor() const = 0;
  virtual float GetDepth() const = 0;
  virtual bool HasCreationAnimation() const = 0;
  virtual bool IsVisible() const { return true; }
};

class UserLineMark
{
public:
  virtual ~UserLineMark() {}

  virtual size_t GetLayerCount() const = 0;
  virtual dp::Color const & GetColor(size_t layerIndex) const = 0;
  virtual float GetWidth(size_t layerIndex) const = 0;
  virtual float GetLayerDepth(size_t layerIndex) const = 0;
  virtual  std::vector<m2::PointD> const & GetPoints() const = 0;
};

class UserMarksProvider
{
public:
  UserMarksProvider();
  virtual ~UserMarksProvider() {}

  bool IsDirty() const;
  virtual bool IsDrawable() const = 0;

  virtual size_t GetUserPointCount() const = 0;
  /// never store UserPointMark reference
  virtual UserPointMark const * GetUserPointMark(size_t index) const = 0;

  virtual size_t GetUserLineCount() const = 0;
  /// never store UserLineMark reference
  virtual UserLineMark const * GetUserLineMark(size_t index) const = 0;

  void IncrementCounter();
  void DecrementCounter();
  bool CanBeDeleted();
  bool IsPendingOnDelete();
  void DeleteLater();

protected:
  void BeginWrite();
  void SetDirty();
  void ResetDirty();
  void EndWrite();

private:
  void Lock();
  void Unlock();

  threads::Mutex m_mutex;
  bool m_isDirty = false;
  std::atomic<bool> m_pendingOnDelete;
  std::atomic<int> m_counter;
};

} // namespace df
