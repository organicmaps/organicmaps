#include "balloon_manager.hpp"
#include "framework.hpp"

#include "../search/result.hpp"

#include "../anim/task.hpp"
#include "../anim/controller.hpp"

#include "../graphics/depth_constants.hpp"
#include "../graphics/opengl/base_texture.hpp"
#include "../graphics/display_list.hpp"
#include "../graphics/icon.hpp"

#include "../geometry/transformations.hpp"

#include "../gui/controller.hpp"

PinClickManager::PinClickManager(Framework & f)
  : m_f(f)
{}

//graphics::DisplayList * PinClickManager::GetSearchPinDL()
//{
//  using namespace graphics;

//  if (!m_searchPinDL)
//  {
//    Screen * cacheScreen = m_f.GetGuiController()->GetCacheScreen();
//    m_searchPinDL = cacheScreen->createDisplayList();

//    cacheScreen->beginFrame();
//    cacheScreen->setDisplayList(m_searchPinDL);

//    Icon::Info infoKey("search-result-active");
//    Resource const * res = cacheScreen->fromID(cacheScreen->findInfo(infoKey));
//    shared_ptr<gl::BaseTexture> texture = cacheScreen->pipeline(res->m_pipelineID).texture();

//    m2::RectU texRect = res->m_texRect;
//    double halfSizeX = texRect.SizeX() / 2.0;
//    double halfSizeY = texRect.SizeY() / 2.0;

//    m2::PointD coords[] =
//    {
//      m2::PointD(-halfSizeX, -halfSizeY),
//      m2::PointD(-halfSizeX, halfSizeY),
//      m2::PointD(halfSizeX, -halfSizeY),
//      m2::PointD(halfSizeX, halfSizeY)
//    };
//    m2::PointF normal(0.0, 0.0);

//    m2::PointF texCoords[] =
//    {
//      texture->mapPixel(m2::PointF(texRect.minX(), texRect.minY())),
//      texture->mapPixel(m2::PointF(texRect.minX(), texRect.maxY())),
//      texture->mapPixel(m2::PointF(texRect.maxX(), texRect.minY())),
//      texture->mapPixel(m2::PointF(texRect.maxX(), texRect.maxY()))
//    };

//    cacheScreen->addTexturedStripStrided(coords, sizeof(m2::PointD),
//                                         &normal, 0,
//                                         texCoords, sizeof(m2::PointF),
//                                         4, graphics::activePinDepth, res->m_pipelineID);

//    cacheScreen->setDisplayList(NULL);
//    cacheScreen->endFrame();
//  }

//  return m_searchPinDL;
//}

void PinClickManager::Hide()
{
  m_f.Invalidate();
}

void PinClickManager::OnClick(m2::PointD const & pxPoint, bool isLongTouch)
{
  UserMark const * mark = m_f.ActivateUserMark(pxPoint, isLongTouch);
  if (mark != NULL)
    OnActivateUserMark(mark);
  SetBalloonVisible(mark != NULL);
}

void PinClickManager::OnBookmarkClick(BookmarkAndCategory const & bnc)
{
  Bookmark * mark = m_f.GetBmCategory(bnc.first)->GetBookmark(bnc.second);
  m_f.GetBookmarkManager().ActivateMark(mark);
  SetBalloonVisible(true);
}

void PinClickManager::SetBalloonVisible(bool isVisible)
{
  if (!isVisible)
    OnDismiss();

  m_f.Invalidate();
}

void PinClickManager::RemovePin()
{
  m_f.GetBookmarkManager().ActivateMark(NULL);
  m_f.Invalidate();
}

void PinClickManager::Dismiss()
{
  OnDismiss();
}

void PinClickManager::ClearListeners()
{
  m_userMarkListener.clear();
  m_dismissListener.clear();
}

void PinClickManager::OnActivateUserMark(UserMark const * mark)
{
  m_userMarkListener(mark);
}

void PinClickManager::OnDismiss()
{
  // Can be called before the listeners will be attached (clearing on activity start).
  if (m_dismissListener)
    m_dismissListener();
}
