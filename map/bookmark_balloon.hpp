#pragma once

#include "../gui/balloon.hpp"
#include "../std/string.hpp"

class Framework;

/// Balloon, which displays information about bookmark
class BookmarkBalloon : public gui::Balloon
{
private:

  m2::PointD m_glbPivot;
  Framework const * m_framework;
  string m_bmkName;

  void update();

public:

  typedef gui::Balloon base_t;

  struct Params : public base_t::Params
  {
    Framework const * m_framework;
  };

  BookmarkBalloon(Params const & p);

  void setGlbPivot(m2::PointD const & pivot);
  m2::PointD const glbPivot();

  void setBookmarkName(string const & name);
  string const & bookmarkName();
};
