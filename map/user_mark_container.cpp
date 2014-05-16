#include "user_mark_container.hpp"

#include "drawer.hpp"
#include "framework.hpp"

#include "../graphics/display_list.hpp"
#include "../graphics/screen.hpp"

#include "../geometry/transformations.hpp"

#include "../anim/task.hpp"
#include "../anim/controller.hpp"

#include "../base/macros.hpp"
#include "../base/stl_add.hpp"

#include "../std/scoped_ptr.hpp"

namespace
{
  struct AnimPhase
  {
    AnimPhase(double endScale, double timeInterval)
      : m_endScale(endScale)
      , m_timeInterval(timeInterval)
    {
    }

    double m_endScale;
    double m_timeInterval;
  };

  class PinAnimation : public anim::Task
  {
  public:
    PinAnimation(Framework & f)
      : m_f(f)
      , m_scale(0.0)
    {
    }

    void AddAnimPhase(AnimPhase const & phase)
    {
      m_animPhases.push_back(phase);
    }

    virtual void OnStart(double ts)
    {
      m_startTime = ts;
      m_startScale = m_scale;
      m_phaseIndex = 0;
    }

    virtual void OnStep(double ts)
    {
      ASSERT(m_phaseIndex < m_animPhases.size(), ());

      AnimPhase const * phase = &m_animPhases[m_phaseIndex];
      double elapsedTime = ts - m_startTime;
      if (elapsedTime > phase->m_timeInterval)
      {
        m_startTime = ts;
        m_scale = phase->m_endScale;
        m_startScale = m_scale;
        m_phaseIndex++;
        if (m_phaseIndex >= m_animPhases.size())
        {
          End();
          return;
        }
      }

      elapsedTime = ts - m_startTime;
      double t = elapsedTime / phase->m_timeInterval;
      m_scale = m_startScale + t * (phase->m_endScale - m_startScale);

      m_f.Invalidate();
    }

    double GetScale() const
    {
      return m_scale;
    }

  private:
    Framework & m_f;
    vector<AnimPhase> m_animPhases;
    size_t m_phaseIndex;
    double m_scale;
    double m_startTime;
    double m_startScale;
  };
}

////////////////////////////////////////////////////////////////////////

namespace
{
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

  void DrawUserMarkImpl(double scale,
                        PaintOverlayEvent const & event,
                        graphics::DisplayList * dl,
                        UserMark const * mark)
  {
    ScreenBase modelView = event.GetModelView();
    graphics::Screen * screen = event.GetDrawer()->screen();
    m2::PointD pxPoint = modelView.GtoP(mark->GetOrg());
    math::Matrix<double, 3, 3> m = math::Shift(math::Scale(math::Identity<double, 3>(),
                                                           scale, scale),
                                               pxPoint.x, pxPoint.y);
    dl->draw(screen, m);
  }

  void DefaultDrawUserMark(double scale,
                           PaintOverlayEvent const & event,
                           UserMarkDLCache * cache,
                           UserMarkDLCache::Key const & defaultKey,
                           UserMark const * mark)
  {
    DrawUserMarkImpl(scale, event, cache->FindUserMark(defaultKey), mark);
  }

  void DrawUserMark(double scale,
                    PaintOverlayEvent const & event,
                    UserMarkDLCache * cache,
                    UserMarkDLCache::Key const & defaultKey,
                    UserMark const * mark)
  {
    if (mark->IsCustomDrawable())
      DrawUserMarkImpl(scale, event, static_cast<ICustomDrawable const *>(mark)->GetDisplayList(cache), mark);
    else
      DefaultDrawUserMark(scale, event, cache, defaultKey, mark);
  }
}

UserMarkContainer::UserMarkContainer(Type type, double layerDepth, Framework & framework)
  : m_controller(this)
  , m_type(type)
  , m_isVisible(true)
  , m_layerDepth(layerDepth)
  , m_activeMark(NULL)
  , m_framework(framework)
{
  ASSERT(m_type >= 0 && m_type < TERMINANT, ());
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
  for_each(m_userMarks.begin(), m_userMarks.end(), bind(&DrawUserMark, 1.0, e, cache, defaultKey, _1));
}

void UserMarkContainer::ActivateMark(UserMark const * mark)
{
  ASSERT(mark->GetContainer() == this, ());
  m_activeMark = mark;
  StartActivationAnim();
}

void UserMarkContainer::DiactivateMark()
{
  KillActivationAnim();
  m_activeMark = NULL;
}

void UserMarkContainer::Clear()
{
  DeleteRange(m_userMarks, DeleteFunctor());
  m_activeMark = NULL;
}

static string s_TypeNames[] =
{
  "search-result",
  "api-result",
  "search-result" // Bookmark active we draw as a search active
};

string UserMarkContainer::GetTypeName() const
{
  ASSERT(m_type < ARRAY_SIZE(s_TypeNames), ());
  return s_TypeNames[m_type];
}

string UserMarkContainer::GetActiveTypeName() const
{
  return GetTypeName() + "-active";
}

UserMark * UserMarkContainer::AllocateUserMark(m2::PointD const & ptOrg)
{
  return new UserMark(ptOrg, this);
}

namespace
{
  class PoiUserMark : public UserMark
  {
  public:
    PoiUserMark(UserMarkContainer * container)
      : UserMark(m2::PointD(0.0, 0.0), container) {}

    void SetPtOrg(m2::PointD const & ptOrg) { m_ptOrg = ptOrg; }
  };

  static scoped_ptr<PoiUserMark> s_selectionUserMark;
}

void UserMarkContainer::InitPoiSelectionMark(UserMarkContainer * container)
{
  ASSERT(s_selectionUserMark == NULL, ());
  s_selectionUserMark.reset(new PoiUserMark(container));
}

UserMark * UserMarkContainer::UserMarkForPoi(m2::PointD const & ptOrg)
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

void UserMarkContainer::EditUserMark(size_t index, UserCustomData * data)
{
  UserMark * mark = GetUserMark(index);
  mark->InjectCustomData(data);
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

void UserMarkContainer::StartActivationAnim()
{
  PinAnimation * anim = new PinAnimation(m_framework);
  anim->AddAnimPhase(AnimPhase(1.2, 0.15));
  anim->AddAnimPhase(AnimPhase(0.8, 0.08));
  anim->AddAnimPhase(AnimPhase(1, 0.05));
  m_animTask.reset(anim);
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
