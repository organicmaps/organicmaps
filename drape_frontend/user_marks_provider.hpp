#pragma once

#include "drape/drape_global.hpp"
#include "drape/pointers.hpp"

#include "geometry/polyline2d.hpp"

#include "base/mutex.hpp"

#include <atomic>
#include <vector>

namespace df
{

class UserPointMark
{
public:
  UserPointMark();
  virtual ~UserPointMark() {}

  virtual bool IsDirty() const = 0;
  virtual void AcceptChanges() const = 0;

  uint32_t GetId() const { return m_id; }

  virtual m2::PointD const & GetPivot() const = 0;
  virtual m2::PointD GetPixelOffset() const = 0;
  virtual std::string GetSymbolName() const  = 0;
  virtual dp::Anchor GetAnchor() const = 0;
  virtual float GetDepth() const = 0;
  virtual bool HasCreationAnimation() const = 0;
  virtual bool IsVisible() const { return true; }

  virtual drape_ptr<dp::TitleDecl> GetTitleDecl() const { return nullptr; }
  virtual uint16_t GetProirity() const { return 0; }

private:
  uint32_t m_id;
};

class UserLineMark
{
public:
  UserLineMark();
  virtual ~UserLineMark() {}

  virtual bool IsDirty() const = 0;
  virtual void AcceptChanges() const = 0;

  virtual uint32_t GetId() const { return m_id; }

  virtual size_t GetLayerCount() const = 0;
  virtual dp::Color const & GetColor(size_t layerIndex) const = 0;
  virtual float GetWidth(size_t layerIndex) const = 0;
  virtual float GetLayerDepth(size_t layerIndex) const = 0;
  virtual std::vector<m2::PointD> const & GetPoints() const = 0;

private:
  uint32_t m_id;
};

class UserMarksProvider
{
public:
  UserMarksProvider();
  virtual ~UserMarksProvider() {}

  virtual bool IsDirty() const = 0;
  virtual void AcceptChanges(std::vector<uint32_t> & removedMarks) = 0;

  virtual bool IsDrawable() const = 0;

  virtual size_t GetUserPointCount() const = 0;
  /// never store UserPointMark reference
  virtual UserPointMark const * GetUserPointMark(size_t index) const = 0;

  virtual size_t GetUserLineCount() const = 0;
  /// never store UserLineMark reference
  virtual UserLineMark const * GetUserLineMark(size_t index) const = 0;

  bool IsPendingOnDelete();
  void DeleteLater();

private:
  bool m_pendingOnDelete;
};

} // namespace df
