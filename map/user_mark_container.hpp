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
  using NotifyChangesFn = std::function<void (UserMarkContainer const &, df::IDCollection const &)>;

  UserMarkContainer(UserMark::Type type);
  virtual ~UserMarkContainer();

  bool IsDirty() const;
  // Discard isDirty and clear the lists of created, removed and updated items.
  virtual void ResetChanges();

  bool IsVisible() const;
  UserMark::Type GetType() const;

  df::MarkIDSet const & GetUserMarks() const { return m_userMarks; }
  virtual df::MarkIDSet const & GetUserLines() const { return m_userLines; }
  df::MarkIDSet const & GetCreatedMarks() const { return m_createdMarks; }
  df::MarkIDSet const & GetUpdatedMarks() const { return m_updatedMarks; }
  df::MarkIDSet const & GetRemovedMarks() const { return m_removedMarks; }

  void AttachUserMark(df::MarkID markId);
  void EditUserMark(df::MarkID markId);
  void DetachUserMark(df::MarkID markId);

  void Clear();
  void SetIsVisible(bool isVisible);
  void Update();

protected:
  void SetDirty();

  UserMark::Type m_type;

  df::MarkIDSet m_userMarks;
  df::MarkIDSet m_userLines;

  df::MarkIDSet m_createdMarks;
  df::MarkIDSet m_removedMarks;
  df::MarkIDSet m_updatedMarks;
  bool m_isVisible = true;
  bool m_isDirty = false;

  DISALLOW_COPY_AND_MOVE(UserMarkContainer);
};

