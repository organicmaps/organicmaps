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

  class DrawFunctor
  {
  public:
    DrawFunctor(double scale,
                ScreenBase const & modelView,
                graphics::DisplayList * dl,
                graphics::Screen * screen)
      : m_scale(scale)
      , m_modelView(modelView)
      , m_dl(dl)
      , m_screen(screen)
    {
    }

    void operator()(UserMark const * mark)
    {
      m2::PointD pxPoint = m_modelView.GtoP(mark->GetOrg());
      math::Matrix<double, 3, 3> m = math::Shift(math::Scale(math::Identity<double, 3>(),
                                                             m_scale, m_scale),
                                                 pxPoint.x, pxPoint.y);
      m_dl->draw(m_screen, m);
    }

  private:
    double m_scale;
    ScreenBase const & m_modelView;
    graphics::DisplayList * m_dl;
    graphics::Screen * m_screen;
  };
}

UserMarkContainer::UserMarkContainer(Type type, double layerDepth, Framework & framework)
  : m_controller(this)
  , m_type(type)
  , m_isVisible(true)
  , m_layerDepth(layerDepth)
  , m_activeMark(NULL)
  , m_symbolDL(NULL)
  , m_activeSymbolDL(NULL)
  , m_cacheScreen(NULL)
  , m_framework(framework)
{
  ASSERT(m_type >= 0 && m_type < TERMINANT, ());
}

UserMarkContainer::~UserMarkContainer()
{
  Clear();
  ReleaseScreen();
}

void UserMarkContainer::SetScreen(graphics::Screen * cacheScreen)
{
  Purge();
  m_cacheScreen = cacheScreen;
}

UserMark const * UserMarkContainer::FindMarkInRect(m2::AnyRectD const & rect, double & d)
{
  UserMark * mark = NULL;
  d = numeric_limits<double>::max();
  FindMarkFunctor f(&mark, d, rect);
  for_each(m_userMarks.begin(), m_userMarks.end(), f);
  return mark;
}

void UserMarkContainer::Draw(PaintOverlayEvent const & e)
{
  if (m_isVisible == false)
    return;

  ASSERT(m_cacheScreen != NULL, ());
  if (m_cacheScreen == NULL)
    return;

  graphics::Screen * screen = e.GetDrawer()->screen();
  ScreenBase modelView = e.GetModelView();
  if (m_activeMark != NULL)
  {
    LOG(LINFO, ("Scale for active mark = ", GetActiveMarkScale()));
    (void)DrawFunctor(GetActiveMarkScale(), modelView, GetActiveDL(), screen)(m_activeMark);
  }

  DrawFunctor f(1.0, modelView, GetDL(), screen);
  for_each(m_userMarks.begin(), m_userMarks.end(), f);
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
  Purge();
  DeleteRange(m_userMarks, DeleteFunctor());
  m_activeMark = NULL;
}

static string s_TypeNames[] =
{
  "search-result",
  "api-result"
};

string const & UserMarkContainer::GetTypeName() const
{
  ASSERT(m_type < ARRAY_SIZE(s_TypeNames), ());
  return s_TypeNames[m_type];
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

UserMark * UserMarkContainer::CreateUserMark(m2::PointD ptOrg)
{
  m_userMarks.push_back(new UserMark(ptOrg, this));
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

graphics::DisplayList * UserMarkContainer::GetDL()
{
  if (m_symbolDL == NULL)
    m_symbolDL = CreateDL(GetTypeName());

  return m_symbolDL;
}

graphics::DisplayList * UserMarkContainer::GetActiveDL()
{
  if (m_activeSymbolDL == NULL)
    m_activeSymbolDL = CreateDL(GetTypeName() + "-active");

  return m_activeSymbolDL;
}

graphics::DisplayList * UserMarkContainer::CreateDL(const string & symbolName)
{
  using namespace graphics;

  graphics::DisplayList * dl = m_cacheScreen->createDisplayList();
  m_cacheScreen->beginFrame();
  m_cacheScreen->setDisplayList(dl);

  Icon::Info infoKey(symbolName);
  Resource const * res = m_cacheScreen->fromID(m_cacheScreen->findInfo(infoKey));
  shared_ptr<gl::BaseTexture> texture = m_cacheScreen->pipeline(res->m_pipelineID).texture();

  m2::RectU texRect = res->m_texRect;
  double halfSizeX = texRect.SizeX() / 2.0;
  double halfSizeY = texRect.SizeY() / 2.0;

  m2::PointD coords[] =
  {
    m2::PointD(-halfSizeX, -halfSizeY),
    m2::PointD(-halfSizeX, halfSizeY),
    m2::PointD(halfSizeX, -halfSizeY),
    m2::PointD(halfSizeX, halfSizeY)
  };
  m2::PointF normal(0.0, 0.0);

  m2::PointF texCoords[] =
  {
    texture->mapPixel(m2::PointF(texRect.minX(), texRect.minY())),
    texture->mapPixel(m2::PointF(texRect.minX(), texRect.maxY())),
    texture->mapPixel(m2::PointF(texRect.maxX(), texRect.minY())),
    texture->mapPixel(m2::PointF(texRect.maxX(), texRect.maxY()))
  };

  m_cacheScreen->addTexturedStripStrided(coords, sizeof(m2::PointD),
                                       &normal, 0,
                                       texCoords, sizeof(m2::PointF),
                                       4, m_layerDepth, res->m_pipelineID);

  m_cacheScreen->setDisplayList(NULL);
  m_cacheScreen->endFrame();

  return dl;
}

void UserMarkContainer::Purge()
{
  delete m_symbolDL;
  m_symbolDL = NULL;
  delete m_activeSymbolDL;
  m_activeSymbolDL = NULL;
}

void UserMarkContainer::ReleaseScreen()
{
  m_cacheScreen = NULL;
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
