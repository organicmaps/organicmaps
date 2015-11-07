#include "user_mark_container.hpp"
#include "framework.hpp"
#include "anim_phase_chain.hpp"

#include "render/drawer.hpp"

#include "graphics/display_list.hpp"
#include "graphics/screen.hpp"
#include "graphics/depth_constants.hpp"

#include "geometry/transformations.hpp"

#include "anim/task.hpp"
#include "anim/controller.hpp"

#include "base/macros.hpp"
#include "base/stl_add.hpp"

#include "std/algorithm.hpp"

////////////////////////////////////////////////////////////////////////

namespace
{
  class PinAnimation : public AnimPhaseChain
  {
  public:
    PinAnimation(Framework & f)
      : AnimPhaseChain(f, m_scale)
      , m_scale(0.0)
    {
      InitDefaultPinAnim(this);
    }

    double GetScale() const
    {
      return m_scale;
    }

  private:
    double m_scale;
  };

  class FindMarkFunctor
  {
  public:
    FindMarkFunctor(UserMark ** mark, double & minD, m2::AnyRectD const & rect)
      : m_mark(mark)
      , m_minD(minD)
      , m_rect(rect)
    {
      m_globalCenter = rect.GlobalCenter();
    }

    void operator()(UserMark * mark)
    {
      m2::PointD const & org = mark->GetOrg();
      if (m_rect.IsPointInside(org))
      {
        double minDCandidate = m_globalCenter.SquareLength(org);
        if (minDCandidate < m_minD)
        {
          *m_mark = mark;
          m_minD = minDCandidate;
        }
      }
    }

    UserMark ** m_mark;
    double & m_minD;
    m2::AnyRectD const & m_rect;
    m2::PointD m_globalCenter;
  };

  void DrawUserMarkByPoint(double scale,
                           double visualScale,
                           m2::PointD const & pixelOfsset,
                           PaintOverlayEvent const & event,
                           graphics::DisplayList * dl,
                           m2::PointD const & ptOrg)
  {
#ifndef USE_DRAPE
    ScreenBase const & modelView = event.GetModelView();
    graphics::Screen * screen = GPUDrawer::GetScreen(event.GetDrawer());
    m2::PointD pxPoint = modelView.GtoP(ptOrg);
    pxPoint += (pixelOfsset * visualScale);
    math::Matrix<double, 3, 3> m = math::Shift(math::Scale(math::Identity<double, 3>(),
                                                           scale, scale),
                                               pxPoint.x, pxPoint.y);
    dl->draw(screen, m);
#endif // USE_DRAPE
  }

  void DrawUserMarkImpl(double scale,
                        double visualScale,
                        m2::PointD const & pixelOfsset,
                        PaintOverlayEvent const & event,
                        graphics::DisplayList * dl,
                        UserMark const * mark)
  {
    DrawUserMarkByPoint(scale, visualScale, pixelOfsset, event, dl, mark->GetOrg());
  }

  void DrawUserMark(double scale,
                    double visualScale,
                    PaintOverlayEvent const & event,
                    UserMarkDLCache * cache,
                    UserMarkDLCache::Key const & defaultKey,
                    UserMark const * mark)
  {
    if (mark->IsCustomDrawable())
    {
      ICustomDrawable const * drawable = static_cast<ICustomDrawable const *>(mark);
      DrawUserMarkImpl(drawable->GetAnimScaleFactor(), visualScale, drawable->GetPixelOffset(), event, drawable->GetDisplayList(cache), mark);
    }
    else
      DrawUserMarkImpl(scale, visualScale, m2::PointD(0.0, 0.0), event, cache->FindUserMark(defaultKey), mark);
  }
}

UserMarkContainer::UserMarkContainer(double layerDepth, Framework & fm)
  : m_framework(fm)
  , m_controller(this)
  , m_isVisible(true)
  , m_isDrawable(true)
  , m_layerDepth(layerDepth)
{
}

UserMarkContainer::~UserMarkContainer()
{
  Clear();
}

template <class ToDo>
void UserMarkContainer::ForEachInRect(m2::RectD const & rect, ToDo toDo) const
{
  for (size_t i = 0; i < m_userMarks.size(); ++i)
    if (rect.IsPointInside(m_userMarks[i]->GetOrg()))
      toDo(m_userMarks[i].get());
}

UserMark const * UserMarkContainer::FindMarkInRect(m2::AnyRectD const & rect, double & d) const
{
  UserMark * mark = NULL;
  if (IsVisible())
  {
    FindMarkFunctor f(&mark, d, rect);
    ForEachInRect(rect.GetGlobalRect(), f);
  }
  return mark;
}

void UserMarkContainer::Draw(PaintOverlayEvent const & e, UserMarkDLCache * cache) const
{
#ifndef USE_DRAPE
  if (IsVisible() && IsDrawable())
  {
    UserMarkDLCache::Key defaultKey(GetTypeName(), graphics::EPosCenter, m_layerDepth);
    ForEachInRect(e.GetClipRect(), bind(&DrawUserMark, 1.0, m_framework.GetVisualScale(),
                                        e, cache, defaultKey, _1));
  }
#endif // USE_DRAPE
}

void UserMarkContainer::Clear(size_t skipCount/* = 0*/)
{
  // Recently added marks stored in the head of list
  // (@see CreateUserMark). Leave tail here.
  if (skipCount < m_userMarks.size())
    m_userMarks.erase(m_userMarks.begin(), m_userMarks.end() - skipCount);
}

namespace
{
  unique_ptr<PoiMarkPoint> g_selectionUserMark;
  unique_ptr<MyPositionMarkPoint> g_myPosition;
}

UserMarkDLCache::Key UserMarkContainer::GetDefaultKey() const
{
  return UserMarkDLCache::Key(GetTypeName(), graphics::EPosCenter, GetDepth());
}

void UserMarkContainer::InitStaticMarks(UserMarkContainer * container)
{
  if (g_selectionUserMark == NULL)
    g_selectionUserMark.reset(new PoiMarkPoint(container));

  if (g_myPosition == NULL)
    g_myPosition.reset(new MyPositionMarkPoint(container));
}

PoiMarkPoint * UserMarkContainer::UserMarkForPoi()
{
  ASSERT(g_selectionUserMark != NULL, ());
  return g_selectionUserMark.get();
}

MyPositionMarkPoint * UserMarkContainer::UserMarkForMyPostion()
{
  ASSERT(g_myPosition != NULL, ());
  return g_myPosition.get();
}

UserMark * UserMarkContainer::CreateUserMark(m2::PointD const & ptOrg)
{
  // Push new marks to the head of list.
  m_userMarks.push_front(unique_ptr<UserMark>(AllocateUserMark(ptOrg)));
  return m_userMarks.front().get();
}

size_t UserMarkContainer::GetUserMarkCount() const
{
  return m_userMarks.size();
}

UserMark const * UserMarkContainer::GetUserMark(size_t index) const
{
  ASSERT_LESS(index, m_userMarks.size(), ());
  return m_userMarks[index].get();
}

UserMark * UserMarkContainer::GetUserMark(size_t index)
{
  ASSERT_LESS(index, m_userMarks.size(), ());
  return m_userMarks[index].get();
}

void UserMarkContainer::DeleteUserMark(size_t index)
{
  ASSERT_LESS(index, m_userMarks.size(), ());
  if (index < m_userMarks.size())
    m_userMarks.erase(m_userMarks.begin() + index);
  else
    LOG(LWARNING, ("Trying to delete non-existing item at index", index));
}

void UserMarkContainer::DeleteUserMark(UserMark const * mark)
{
  size_t index = FindUserMark(mark);
  if (index != m_userMarks.size())
    DeleteUserMark(index);
}

size_t UserMarkContainer::FindUserMark(UserMark const * mark)
{
  auto it = find_if(m_userMarks.begin(), m_userMarks.end(), [&mark](unique_ptr<UserMark> const & p)
                    {
                      return p.get() == mark;
                    });
  return distance(m_userMarks.begin(), it);
}

SearchUserMarkContainer::SearchUserMarkContainer(double layerDepth, Framework & framework)
  : UserMarkContainer(layerDepth, framework)
{
}

string SearchUserMarkContainer::GetTypeName() const
{
  return "search-result";
}

string SearchUserMarkContainer::GetActiveTypeName() const
{
  return "search-result-active";
}

UserMark * SearchUserMarkContainer::AllocateUserMark(const m2::PointD & ptOrg)
{
  return new SearchMarkPoint(ptOrg, this);
}

DebugUserMarkContainer::DebugUserMarkContainer(double layerDepth, Framework & framework)
  : UserMarkContainer(layerDepth, framework)
{
}

string DebugUserMarkContainer::GetTypeName() const
{
  // api-result.png is reused for debug markers
  return "api-result";
}

string DebugUserMarkContainer::GetActiveTypeName() const
{
  // api-result.png is reused for debug markers
  return "api-result";
}

UserMark * DebugUserMarkContainer::AllocateUserMark(const m2::PointD & ptOrg)
{
  return new DebugMarkPoint(ptOrg, this);
}


SelectionContainer::SelectionContainer(Framework & fm)
  : m_container(NULL)
  , m_fm(fm)
{
}

void SelectionContainer::ActivateMark(UserMark const * userMark, bool needAnim)
{
  if (needAnim)
    KillActivationAnim();
  if (userMark != NULL)
  {
    m_ptOrg = userMark->GetOrg();
    m_container = userMark->GetContainer();
    if (needAnim)
      StartActivationAnim();
  }
  else
    m_container = NULL;
}

void SelectionContainer::Draw(const PaintOverlayEvent & e, UserMarkDLCache * cache) const
{
  if (m_container != NULL)
  {
    UserMarkDLCache::Key defaultKey(m_container->GetActiveTypeName(),
                                    graphics::EPosCenter,
                                    graphics::activePinDepth);

    DrawUserMarkByPoint(GetActiveMarkScale(),
                        m_fm.GetVisualScale(),
                        m2::PointD(0, 0),
                        e, cache->FindUserMark(defaultKey),
                        m_ptOrg);
  }
}

bool SelectionContainer::IsActive() const
{
  return m_container != NULL;
}

void SelectionContainer::StartActivationAnim()
{
  m_animTask.reset(new PinAnimation(m_fm));
  m_fm.GetAnimController()->AddTask(m_animTask);
  m_fm.Invalidate();
}

void SelectionContainer::KillActivationAnim()
{
  m_animTask.reset();
}

double SelectionContainer::GetActiveMarkScale() const
{
  if (m_animTask != NULL)
  {
    PinAnimation * a = static_cast<PinAnimation *>(m_animTask.get());
    return a->GetScale();
  }

  return 1.0;
}
