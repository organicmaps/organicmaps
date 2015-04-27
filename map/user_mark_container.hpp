#pragma once

#include "map/events.hpp"
#include "map/user_mark.hpp"
#include "map/user_mark_dl_cache.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "std/deque.hpp"
#include "std/noncopyable.hpp"
#include "std/unique_ptr.hpp"

class Framework;

namespace anim
{
  class Task;
}

namespace graphics
{
  class DisplayList;
  class Screen;
}

class UserMarkContainer : private noncopyable
{
public:
  using UserMarksListT = deque<unique_ptr<UserMark>>;

  class Controller
  {
  public:
    Controller(UserMarkContainer * container)
      : m_container(container) {}

    UserMark * CreateUserMark(m2::PointD const & ptOrg) { return m_container->CreateUserMark(ptOrg); }
    size_t GetUserMarkCount() const { return m_container->GetUserMarkCount(); }
    UserMark const * GetUserMark(size_t index) const { return m_container->GetUserMark(index); }
    UserMark * GetUserMarkForEdit(size_t index) { return m_container->GetUserMark(index); }
    void DeleteUserMark(size_t index) { m_container->DeleteUserMark(index); }
    void DeleteUserMark(UserMark const * mark) { m_container->DeleteUserMark(mark); }

    // Returns index of the mark if exists, otherwise returns
    // number of user marks.
    size_t FindUserMark(UserMark const * mark) const { return m_container->FindUserMark(mark); }

  private:
    UserMarkContainer * m_container;
  };

  enum Type
  {
    SEARCH_MARK,
    API_MARK,
    DEBUG_MARK,
    BOOKMARK_MARK
  };

  UserMarkContainer(double layerDepth, Framework & fm);
  virtual ~UserMarkContainer();

  void SetScreen(graphics::Screen * cacheScreen);
  virtual Type GetType() const = 0;

  bool IsVisible() const { return m_isVisible; }
  void SetVisible(bool isVisible) { m_isVisible = isVisible; }

  bool IsDrawable() const { return m_isDrawable; }
  void SetIsDrawable(bool isDrawable) { m_isDrawable = isDrawable; }

  // If not found mark on rect result is NULL
  // If mark is found in "d" return distance from rect center
  // In multiple select choose mark with min(d)
  UserMark const * FindMarkInRect(m2::AnyRectD const & rect, double & d) const;

  void Draw(PaintOverlayEvent const & e, UserMarkDLCache * cache) const;
  void ActivateMark(UserMark const * mark);
  void DiactivateMark();

  void Clear(size_t skipCount = 0);

  double GetDepth() const { return m_layerDepth; }

  static void InitStaticMarks(UserMarkContainer * container);
  static PoiMarkPoint * UserMarkForPoi();
  static MyPositionMarkPoint * UserMarkForMyPostion();

  Controller const & GetController() const { return m_controller; }
  Controller & GetController() { return m_controller; }

  virtual string GetActiveTypeName() const = 0;

protected:
  virtual string GetTypeName() const = 0;
  virtual UserMark * AllocateUserMark(m2::PointD const & ptOrg) = 0;

private:
  friend class Controller;
  UserMark * CreateUserMark(m2::PointD const & ptOrg);
  size_t GetUserMarkCount() const;
  UserMark const * GetUserMark(size_t index) const;
  UserMark * GetUserMark(size_t index);
  void DeleteUserMark(size_t index);
  void DeleteUserMark(UserMark const * mark);
  size_t FindUserMark(UserMark const * mark);

  template <class ToDo> void ForEachInRect(m2::RectD const & rect, ToDo toDo) const;

protected:
  Framework & m_framework;

private:
  Controller m_controller;
  bool m_isVisible;
  bool m_isDrawable;
  double m_layerDepth;
  UserMarksListT m_userMarks;
};

class SearchUserMarkContainer : public UserMarkContainer
{
public:
  SearchUserMarkContainer(double layerDepth, Framework & framework);

  virtual Type GetType() const { return SEARCH_MARK; }

  virtual string GetActiveTypeName() const;
protected:
  virtual string GetTypeName() const;
  virtual UserMark * AllocateUserMark(m2::PointD const & ptOrg);
};

class ApiUserMarkContainer : public UserMarkContainer
{
public:
  ApiUserMarkContainer(double layerDepth, Framework & framework);

  virtual Type GetType() const { return API_MARK; }

  virtual string GetActiveTypeName() const;
protected:
  virtual string GetTypeName() const;
  virtual UserMark * AllocateUserMark(m2::PointD const & ptOrg);
};

class DebugUserMarkContainer : public UserMarkContainer
{
public:
  DebugUserMarkContainer(double layerDepth, Framework & framework);

  virtual Type GetType() const { return DEBUG_MARK; }

  virtual string GetActiveTypeName() const;
protected:
  virtual string GetTypeName() const;
  virtual UserMark * AllocateUserMark(m2::PointD const & ptOrg);
};

class SelectionContainer
{
public:
  SelectionContainer(Framework & fm);

  void ActivateMark(UserMark const * userMark, bool needAnim);
  void Draw(PaintOverlayEvent const & e, UserMarkDLCache * cache) const;
  bool IsActive() const;

private:
  /// animation support
  void StartActivationAnim();
  void KillActivationAnim();
  double GetActiveMarkScale() const;

  shared_ptr<anim::Task> m_animTask;

private:
  friend class BookmarkManager;
  UserMarkContainer const * m_container;
  m2::PointD m_ptOrg;
  Framework & m_fm;
};
