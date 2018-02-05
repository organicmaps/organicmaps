#pragma once

#include "map/user_mark.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/any_rect2d.hpp"

#include <base/macros.hpp>

#include <functional>
#include <memory>
#include <set>

class UserMarkContainer
{
public:
  using MarkIDSet = std::set<df::MarkID>;
  using NotifyChangesFn = std::function<void (UserMarkContainer const &, df::IDCollection const &)>;

  UserMarkContainer(UserMark::Type type);
  virtual ~UserMarkContainer();

  size_t GetUserPointCount() const;
  virtual size_t GetUserLineCount() const;
  bool IsDirty() const;

  // Discard isDirty flag, return id collection of removed marks since previous method call.
  virtual void AcceptChanges(df::MarkIDCollection & groupMarks,
                     df::MarkIDCollection & createdMarks,
                     df::MarkIDCollection & removedMarks);

  bool IsVisible() const;
  size_t GetUserMarkCount() const;
  UserMark::Type GetType() const;

  MarkIDSet const & GetUserMarks() const { return m_userMarks; }
  MarkIDSet const & GetCreatedMarks() const { return m_createdMarks; }
  MarkIDSet const & GetUpdatedMarks() const { return m_updatedMarks; }
  MarkIDSet const & GetRemovedMarks() const { return m_removedMarks; }

  void AttachUserMark(df::MarkID markId);
  void EditUserMark(df::MarkID markId);
  void DetachUserMark(df::MarkID markId);

  void Clear();
  void SetIsVisible(bool isVisible);
  void Update();

protected:
  void SetDirty();

  UserMark::Type m_type;

  MarkIDSet m_userMarks;
  MarkIDSet m_userLines;

  MarkIDSet m_createdMarks;
  MarkIDSet m_removedMarks;
  MarkIDSet m_updatedMarks;
  bool m_isVisible = true;
  bool m_isDirty = false;

  DISALLOW_COPY_AND_MOVE(UserMarkContainer);
};

