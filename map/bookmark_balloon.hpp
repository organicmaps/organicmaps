#pragma once

#include "../gui/balloon.hpp"
#include "../std/string.hpp"

class Framework;

/// Balloon, which displays information about bookmark
class BookmarkBalloon : public gui::Balloon
{
private:

  class BalloonAnimTask;
  shared_ptr<BalloonAnimTask> m_currentAnimTask;

  m2::PointD m_glbPivot;
  Framework const * m_framework;
  string m_bmkName;
  string m_bmkType;

  void createTask(double startScale, double endScale,
                  double startOffset, double endOffset,
                  double interval, int index);
  void animTaskEnded(int animIndex);
  void cancelTask();

  void update();

public:

  typedef gui::Balloon base_t;

  struct Params : public base_t::Params
  {
    Framework const * m_framework;
  };

  BookmarkBalloon(Params const & p);

  void showAnimated();
  void hide();

  void setGlbPivot(m2::PointD const & pivot);
  m2::PointD const glbPivot();

  void setBookmarkCaption(string const & name, string const & type);
  string const & bookmarkName();
};
