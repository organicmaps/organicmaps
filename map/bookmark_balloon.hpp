#pragma once

#include "../gui/balloon.hpp"

class Framework;

namespace gui
{
  class BookmarkBalloon : public Balloon
  {
  private:
    void update();
    m2::PointD m_bookmarkPivot;
    Framework const * m_framework;
    std::string m_bookmarkName;

  public:
    BookmarkBalloon(Params const & p, Framework const * framework);

    void setBookmarkPivot(m2::PointD const & pivot);
    m2::PointD const getBookmarkPivot();
    void setBookmarkName(std::string const & name);
    std::string const getBookmarkName();
  };
}
