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
  UserMarkContainer(UserMark::Type type);
  virtual ~UserMarkContainer();

  bool IsDirty() const { return m_isDirty; }
  virtual void ResetChanges();

  bool IsVisible() const;
  bool IsVisibilityChanged() const;
  UserMark::Type GetType() const;

  df::MarkIDSet const & GetUserMarks() const { return m_userMarks; }
  virtual df::MarkIDSet const & GetUserLines() const { return m_userLines; }

  void AttachUserMark(df::MarkID markId);
  void DetachUserMark(df::MarkID markId);

  void Clear();
  void SetIsVisible(bool isVisible);

protected:
  void SetDirty() { m_isDirty = true; }

  UserMark::Type m_type;

  df::MarkIDSet m_userMarks;
  df::MarkIDSet m_userLines;

  bool m_isDirty = false;
  bool m_isVisible = true;
  bool m_wasVisible = false;

  DISALLOW_COPY_AND_MOVE(UserMarkContainer);
};

