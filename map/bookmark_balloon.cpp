#include "bookmark_balloon.hpp"
#include "framework.hpp"

#define POPUP_PADDING 23

BookmarkBalloon::BookmarkBalloon(Params const & p)
 : Balloon(p),
   m_framework(p.m_framework)
{
}

void BookmarkBalloon::update()
{
  Balloon::update();
  if (isVisible())
  {
    m2::PointD newPivot(m_framework->GtoP(m_glbPivot));
    newPivot.y -= POPUP_PADDING * visualScale();
    setPivot(newPivot);
  }
}

void BookmarkBalloon::setGlbPivot(m2::PointD const & pivot)
{
  m_glbPivot = pivot;
}

m2::PointD const BookmarkBalloon::glbPivot()
{
  return m_glbPivot;
}

void BookmarkBalloon::setBookmarkName(string const & name)
{
  m_bookmarkName = name;
  setText(name);//.substr(0, 13));
}

string const & BookmarkBalloon::bookmarkName()
{
  return m_bookmarkName;
}
