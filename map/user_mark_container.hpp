#pragma once

#include "events.hpp"
#include "user_mark.hpp"

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
  ~UserMarkContainer();

  void SetScreen(graphics::Screen * cacheScreen);

  bool IsVisible() const { return m_isVisible; }
  void SetVisible(bool isVisible) { m_isVisible = isVisible; }

  // If not found mark on rect result is NULL
  // If mark is found in "d" return distance from rect center
  // In multiple select choose mark with min(d)
  UserMark const * FindMarkInRect(m2::AnyRectD const & rect, double & d);

  void Draw(PaintOverlayEvent const & e);
  void ActivateMark(UserMark const * mark);
  void DiactivateMark();

  void Clear();

  Type const & GetType() const { return m_type; }
  string const & GetTypeName() const;

  static void InitPoiSelectionMark(UserMarkContainer * container);
  static UserMark * UserMarkForPoi(m2::PointD const & ptOrg);

  Controller const & GetController() const { return m_controller; }
  Controller & GetController() { return m_controller; }

private:
  friend class Controller;
  UserMark * CreateUserMark(m2::PointD ptOrg);
  size_t GetUserMarkCount() const;
  UserMark const * GetUserMark(size_t index) const;
  UserMark * GetUserMark(size_t index);
  void EditUserMark(size_t index, UserCustomData * data);
  void DeleteUserMark(size_t index);

private:
  graphics::DisplayList * GetDL();
  graphics::DisplayList * GetActiveDL();
  graphics::DisplayList * CreateDL(string const & symbolName);
  void Purge();
  void ReleaseScreen();

private:
  Controller m_controller;
  Type m_type;
  bool m_isVisible;
  double m_layerDepth;
  vector<UserMark *> m_userMarks;
  UserMark const * m_activeMark;
  graphics::DisplayList * m_symbolDL;
  graphics::DisplayList * m_activeSymbolDL;
  graphics::Screen * m_cacheScreen;

private:
  /// animation support
  void StartActivationAnim();
  void KillActivationAnim();
  double GetActiveMarkScale() const;

  Framework & m_framework;
  shared_ptr<anim::Task> m_animTask;
};
