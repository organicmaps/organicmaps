#pragma once

#include "events.hpp"
#include "user_mark.hpp"
#include "user_mark_dl_cache.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "../std/noncopyable.hpp"

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
  class Controller
  {
  public:
    Controller(UserMarkContainer * container)
      : m_container(container) {}

    UserMark * CreateUserMark(m2::PointD ptOrg) { return m_container->CreateUserMark(ptOrg); }
    size_t GetUserMarkCount() const { return m_container->GetUserMarkCount(); }
    UserMark const * GetUserMark(size_t index) const { return m_container->GetUserMark(index); }
    void EditUserMark(size_t index, UserCustomData * data) { m_container->EditUserMark(index, data); }
    void DeleteUserMark(size_t index) { m_container->DeleteUserMark(index); }

  private:
    UserMarkContainer * m_container;
  };

  enum Type
  {
    SEARCH_MARK,
    API_MARK,
    BOOKMARK_MARK,
    TERMINANT
  };

  UserMarkContainer(Type type, double layerDepth, Framework & framework);
  virtual ~UserMarkContainer();

  void SetScreen(graphics::Screen * cacheScreen);

  bool IsVisible() const { return m_isVisible; }
  void SetVisible(bool isVisible) { m_isVisible = isVisible; }

  // If not found mark on rect result is NULL
  // If mark is found in "d" return distance from rect center
  // In multiple select choose mark with min(d)
  UserMark const * FindMarkInRect(m2::AnyRectD const & rect, double & d);

  void Draw(PaintOverlayEvent const & e, UserMarkDLCache * cache) const;
  void ActivateMark(UserMark const * mark);
  void DiactivateMark();

  void Clear();

  Type const & GetType() const { return m_type; }
  double GetDepth() const { return m_layerDepth; }

  static void InitPoiSelectionMark(UserMarkContainer * container);
  static UserMark * UserMarkForPoi(m2::PointD const & ptOrg);

  Controller const & GetController() const { return m_controller; }
  Controller & GetController() { return m_controller; }

protected:
  virtual string GetTypeName() const;
  virtual string GetActiveTypeName() const;
  virtual UserMark * AllocateUserMark(m2::PointD const & ptOrg);

private:
  friend class Controller;
  UserMark * CreateUserMark(m2::PointD const & ptOrg);
  size_t GetUserMarkCount() const;
  UserMark const * GetUserMark(size_t index) const;
  UserMark * GetUserMark(size_t index);
  void EditUserMark(size_t index, UserCustomData * data);
  void DeleteUserMark(size_t index);

private:
  Controller m_controller;
  Type m_type;
  bool m_isVisible;
  double m_layerDepth;
  vector<UserMark *> m_userMarks;
  UserMark const * m_activeMark;

private:
  /// animation support
  void StartActivationAnim();
  void KillActivationAnim();
  double GetActiveMarkScale() const;

  Framework & m_framework;
  shared_ptr<anim::Task> m_animTask;
};
