#include "bookmark_balloon.hpp"
#include "framework.hpp"

#define POPUP_PADDING 23

namespace gui
{
  BookmarkBalloon::BookmarkBalloon(Params const & p, Framework * framework)
  : Balloon(p),
    m_framework(framework)
  {
  }

  void BookmarkBalloon::update()
  {
    Balloon::update();
    if (isVisible())
    {
      m2::PointD newPivot(m_framework->GtoP(m_bookmarkPivot));
      newPivot.y -= POPUP_PADDING * visualScale();
      setPivot(newPivot);
    }
  }

  void BookmarkBalloon::setBookmarkPivot(m2::PointD const & pivot)
  {
    m_bookmarkPivot = pivot;
  }

  m2::PointD const BookmarkBalloon::getBookmarkPivot()
  {
    return m_bookmarkPivot;
  }

  void BookmarkBalloon::setBookmarkName(std::string const & name)
  {
    m_bookmarkName = name;
    setText(name);//.substr(0, 13));
  }

  std::string const BookmarkBalloon::getBookmarkName()
  {
    return m_bookmarkName;
  }
}
