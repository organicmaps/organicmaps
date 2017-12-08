#pragma once

#include "map/user_mark.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"
#include "drape_frontend/user_marks_provider.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/any_rect2d.hpp"

#include <base/macros.hpp>

#include <bitset>
#include <deque>
#include <functional>
#include <memory>
#include <set>

class UserMarksController
{
public:
  virtual size_t GetUserMarkCount() const = 0;
  virtual UserMark::Type GetType() const = 0;
  virtual void SetIsDrawable(bool isDrawable) = 0;
  virtual void SetIsVisible(bool isVisible) = 0;

  virtual UserMark * CreateUserMark(m2::PointD const & ptOrg) = 0;
  virtual UserMark const * GetUserMark(size_t index) const = 0;
  virtual UserMark * GetUserMarkForEdit(size_t index) = 0;
  virtual void DeleteUserMark(size_t index) = 0;
  virtual void Clear() = 0;
  virtual void Update() = 0;
  virtual void NotifyChanges() = 0;
};

class UserMarkContainer : public df::UserMarksProvider
                        , public UserMarksController
{
public:
  using TUserMarksList = std::deque<std::unique_ptr<UserMark>>;
  using NotifyChangesFn = std::function<void (UserMarkContainer *, df::IDCollection const &)>;

  struct Listeners
  {
    Listeners() = default;
    Listeners(NotifyChangesFn const & createListener,
              NotifyChangesFn const & updateListener,
              NotifyChangesFn const & deleteListener)
      : m_createListener(createListener)
      , m_updateListener(updateListener)
      , m_deleteListener(deleteListener)
    {}

    NotifyChangesFn m_createListener;
    NotifyChangesFn m_updateListener;
    NotifyChangesFn m_deleteListener;
  };

  UserMarkContainer(double layerDepth, UserMark::Type type,
                    Listeners const & listeners = Listeners());
  ~UserMarkContainer() override;

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void SetListeners(Listeners const & listeners);
  UserMark const * GetUserMarkById(df::MarkID id) const;

  // If not found mark on rect result is nullptr.
  // If mark is found in "d" return distance from rect center.
  // In multiple select choose mark with min(d).
  UserMark const * FindMarkInRect(m2::AnyRectD const & rect, double & d) const;

  // UserMarksProvider implementation.
  size_t GetUserPointCount() const override;
  df::UserPointMark const * GetUserPointMark(size_t index) const override;

  size_t GetUserLineCount() const override;
  df::UserLineMark const * GetUserLineMark(size_t index) const override;

  bool IsDirty() const override;

  // Discard isDirty flag, return id collection of removed marks since previous method call.
  void AcceptChanges(df::MarkIDCollection & createdMarks,
                     df::MarkIDCollection & removedMarks) override;

  float GetPointDepth() const;

  bool IsVisible() const;
  bool IsDrawable() const override;
  size_t GetUserMarkCount() const override;
  UserMark const * GetUserMark(size_t index) const override;
  UserMark::Type GetType() const override final;

  // UserMarksController implementation.
  UserMark * CreateUserMark(m2::PointD const & ptOrg) override;
  UserMark * GetUserMarkForEdit(size_t index) override;
  void DeleteUserMark(size_t index) override;
  void Clear() override;
  void SetIsDrawable(bool isDrawable) override;
  void SetIsVisible(bool isVisible) override;
  void Update() override;
  void NotifyChanges() override;

protected:
  void SetDirty();

  virtual UserMark * AllocateUserMark(m2::PointD const & ptOrg) = 0;

private:
  void NotifyListeners();

  df::DrapeEngineSafePtr m_drapeEngine;
  std::bitset<4> m_flags;
  double m_layerDepth;
  TUserMarksList m_userMarks;
  UserMark::Type m_type;
  std::set<df::MarkID> m_createdMarks;
  std::set<df::MarkID> m_removedMarks;
  bool m_isDirty = false;

  Listeners m_listeners;

  DISALLOW_COPY_AND_MOVE(UserMarkContainer);
};

template<typename MarkPointClassType, UserMark::Type UserMarkType>
class SpecifiedUserMarkContainer : public UserMarkContainer
{
public:
  explicit SpecifiedUserMarkContainer()
    : UserMarkContainer(0.0 /* layer depth */, UserMarkType)
  {}

protected:
  UserMark * AllocateUserMark(m2::PointD const & ptOrg) override
  {
    return new MarkPointClassType(ptOrg, this);
  }
};
