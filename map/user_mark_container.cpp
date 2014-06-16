#include "user_mark_container.hpp"

#include "drawer.hpp"
#include "framework.hpp"
#include "anim_phase_chain.hpp"

#include "../graphics/display_list.hpp"
#include "../graphics/screen.hpp"

#include "../geometry/transformations.hpp"

#include "../anim/task.hpp"
#include "../anim/controller.hpp"

#include "../base/macros.hpp"
#include "../base/stl_add.hpp"

#include "../std/scoped_ptr.hpp"
#include "../std/algorithm.hpp"

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
    ScreenBase const & modelView = event.GetModelView();
    graphics::Screen * screen = event.GetDrawer()->screen();
    m2::PointD pxPoint = modelView.GtoP(ptOrg);
    pxPoint += (pixelOfsset * visualScale);
    math::Matrix<double, 3, 3> m = math::Shift(math::Scale(math::Identity<double, 3>(),
                                                           scale, scale),
                                               pxPoint.x, pxPoint.y);
    dl->draw(screen, m);
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

UserMark const * UserMarkContainer::FindMarkInRect(m2::AnyRectD const & rect, double & d) const
{
  UserMark * mark = NULL;
  if (IsVisible())
  {
    FindMarkFunctor f(&mark, d, rect);
    for_each(m_userMarks.begin(), m_userMarks.end(), f);
  }
  return mark;
}

void UserMarkContainer::Draw(PaintOverlayEvent const & e, UserMarkDLCache * cache) const
{
  if (IsVisible() && IsDrawable())
  {
    UserMarkDLCache::Key defaultKey(GetTypeName(), graphics::EPosCenter, m_layerDepth);
    for_each(m_userMarks.begin(), m_userMarks.end(), bind(&DrawUserMark, 1.0, m_framework.GetVisualScale(),
                                                          e, cache, defaultKey, _1));
  }
}

void UserMarkContainer::Clear()
{
  DeleteRange(m_userMarks, DeleteFunctor());
}

namespace
{
  static scoped_ptr<PoiMarkPoint> s_selectionUserMark;
}

void UserMarkContainer::InitPoiSelectionMark(UserMarkContainer * container)
{
  ASSERT(s_selectionUserMark == NULL, ());
  s_selectionUserMark.reset(new PoiMarkPoint(container));
}

PoiMarkPoint * UserMarkContainer::UserMarkForPoi()
{
  ASSERT(s_selectionUserMark != NULL, ());
  return s_selectionUserMark.get();
}

UserMark * UserMarkContainer::CreateUserMark(m2::PointD const & ptOrg)
{
  m_userMarks.push_back(AllocateUserMark(ptOrg));
  return m_userMarks.back();
}

size_t UserMarkContainer::GetUserMarkCount() const
{
  return m_userMarks.size();
}

UserMark const * UserMarkContainer::GetUserMark(size_t index) const
{
  ASSERT_LESS(index, m_userMarks.size(), ());
  return m_userMarks[index];
}

UserMark * UserMarkContainer::GetUserMark(size_t index)
{
  ASSERT_LESS(index, m_userMarks.size(), ());
  return m_userMarks[index];
}

namespace
{

template <class T> void DeleteItem(vector<T> & v, size_t i)
{
  if (i < v.size())
  {
    delete v[i];
    v.erase(v.begin() + i);
  }
  else
  {
    LOG(LWARNING, ("Trying to delete non-existing item at index", i));
  }
}

}

void UserMarkContainer::DeleteUserMark(size_t index)
{
  ASSERT_LESS(index, m_userMarks.size(), ());
  DeleteItem(m_userMarks, index);
}

void UserMarkContainer::DeleteUserMark(UserMark const * mark)
{
  vector<UserMark *>::iterator it = find(m_userMarks.begin(), m_userMarks.end(), mark);
  if (it != m_userMarks.end())
    DeleteUserMark(distance(m_userMarks.begin(), it));
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

ApiUserMarkContainer::ApiUserMarkContainer(double layerDepth, Framework & framework)
  : UserMarkContainer(layerDepth, framework)
{
}

string ApiUserMarkContainer::GetTypeName() const
{
  return "api-result";
}

string ApiUserMarkContainer::GetActiveTypeName() const
{
  return "search-result-active";
}

UserMark * ApiUserMarkContainer::AllocateUserMark(const m2::PointD & ptOrg)
{
  return new ApiMarkPoint(ptOrg, this);
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
                                    m_container->GetDepth());

    DrawUserMarkByPoint(GetActiveMarkScale(),
                        m_fm.GetVisualScale(),
                        m2::PointD(0, 0),
                        e, cache->FindUserMark(defaultKey),
                        m_ptOrg);
  }
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
