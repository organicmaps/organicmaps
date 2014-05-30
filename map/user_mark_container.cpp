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
}

UserMarkContainer::UserMarkContainer(double layerDepth, Framework & framework)
  : m_controller(this)
  , m_isVisible(true)
  , m_layerDepth(layerDepth)
  , m_activeMark(NULL)
  , m_framework(framework)
{
}

UserMarkContainer::~UserMarkContainer()
{
  Clear();
}

UserMark const * UserMarkContainer::FindMarkInRect(m2::AnyRectD const & rect, double & d)
{
  UserMark * mark = NULL;
  d = numeric_limits<double>::max();
  FindMarkFunctor f(&mark, d, rect);
  for_each(m_userMarks.begin(), m_userMarks.end(), f);
  return mark;
}

void UserMarkContainer::Draw(PaintOverlayEvent const & e, UserMarkDLCache * cache) const
{
  if (m_isVisible == false)
    return;

  if (m_activeMark != NULL)
  {
    UserMarkDLCache::Key defaultKey(GetActiveTypeName(), graphics::EPosCenter, m_layerDepth);
    DefaultDrawUserMark(GetActiveMarkScale(), e, cache, defaultKey, m_activeMark);
  }

  UserMarkDLCache::Key defaultKey(GetTypeName(), graphics::EPosCenter, m_layerDepth);
  for_each(m_userMarks.begin(), m_userMarks.end(), bind(&UserMarkContainer::DrawUserMark, this,
                                                         1.0, e, cache, defaultKey, _1));
}

void UserMarkContainer::ActivateMark(UserMark const * mark)
{
  ASSERT(mark->GetContainer() == this, ());
  m_activeMark = mark;
  StartActivationAnim();
}

void UserMarkContainer::DiactivateMark()
{
  if (m_activeMark != NULL)
  {
    KillActivationAnim();
    m_activeMark = NULL;
  }
}

void UserMarkContainer::Clear()
{
  DeleteRange(m_userMarks, DeleteFunctor());
  m_activeMark = NULL;
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

PoiMarkPoint * UserMarkContainer::UserMarkForPoi(m2::PointD const & ptOrg)
{
  ASSERT(s_selectionUserMark != NULL, ());
  s_selectionUserMark->SetPtOrg(ptOrg);
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
  if (m_activeMark == m_userMarks[index])
    m_activeMark = NULL;

  DeleteItem(m_userMarks, index);
}

void UserMarkContainer::DeleteUserMark(UserMark const * mark)
{
  vector<UserMark *>::iterator it = find(m_userMarks.begin(), m_userMarks.end(), mark);
  if (it != m_userMarks.end())
    DeleteUserMark(distance(m_userMarks.begin(), it));
}

void UserMarkContainer::DrawUserMarkImpl(double scale,
                                         m2::PointD const & pixelOfsset,
                                         PaintOverlayEvent const & event,
                                         graphics::DisplayList * dl,
                                         UserMark const * mark) const
{
  ScreenBase modelView = event.GetModelView();
  graphics::Screen * screen = event.GetDrawer()->screen();
  m2::PointD pxPoint = modelView.GtoP(mark->GetOrg());
  pxPoint += (pixelOfsset * m_framework.GetVisualScale());
  math::Matrix<double, 3, 3> m = math::Shift(math::Scale(math::Identity<double, 3>(),
                                                         scale, scale),
                                             pxPoint.x, pxPoint.y);
  dl->draw(screen, m);
}

void UserMarkContainer::DrawUserMark(double scale,
                                     PaintOverlayEvent const & event,
                                     UserMarkDLCache * cache,
                                     UserMarkDLCache::Key const & defaultKey,
                                     UserMark const * mark) const
{
  if (mark->IsCustomDrawable())
  {
    ICustomDrawable const * drawable = static_cast<ICustomDrawable const *>(mark);
    DrawUserMarkImpl(drawable->GetAnimScaleFactor(), drawable->GetPixelOffset(), event, drawable->GetDisplayList(cache), mark);
  }
  else
    DefaultDrawUserMark(scale, event, cache, defaultKey, mark);
}

void UserMarkContainer::DefaultDrawUserMark(double scale,
                                            PaintOverlayEvent const & event,
                                            UserMarkDLCache * cache,
                                            UserMarkDLCache::Key const & defaultKey,
                                            const UserMark * mark) const
{
  DrawUserMarkImpl(scale, m2::PointD(0.0, 0.0), event, cache->FindUserMark(defaultKey), mark);
}

void UserMarkContainer::StartActivationAnim()
{
  m_animTask.reset(new PinAnimation(m_framework));
  m_framework.GetAnimController()->AddTask(m_animTask);
}

void UserMarkContainer::KillActivationAnim()
{
  m_animTask.reset();
}

double UserMarkContainer::GetActiveMarkScale() const
{
  if (m_animTask != NULL)
  {
    PinAnimation * a = static_cast<PinAnimation *>(m_animTask.get());
    return a->GetScale();
  }

  return 1.0;
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
