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

PinClickManager::PinClickManager(Framework & f)
  : m_searchPinDL(NULL)
  , m_f(f)
  , m_updateForLocation(false)
  , m_hasPin(false)
{}

PinClickManager::~PinClickManager()
{
  ASSERT(m_searchPinDL == NULL, ());
}

void PinClickManager::Shutdown()
{
  delete m_searchPinDL;
  m_searchPinDL = NULL;
}

void PinClickManager::RenderPolicyCreated(graphics::EDensity density)
{
}

void PinClickManager::LocationChanged(location::GpsInfo const & info)
{}

void PinClickManager::StartAnimation()
{
  PinAnimation * anim = new PinAnimation(m_f);
  anim->AddAnimPhase(AnimPhase(1.2, 0.15));
  anim->AddAnimPhase(AnimPhase(0.8, 0.08));
  anim->AddAnimPhase(AnimPhase(1, 0.05));
  m_animTask.reset(anim);
  m_f.GetAnimController()->AddTask(m_animTask);
}

void PinClickManager::ResetAnimation()
{
  m_animTask.reset();
}

double PinClickManager::GetCurrentPinScale()
{
  if (m_animTask != NULL)
  {
    PinAnimation * a = static_cast<PinAnimation *>(m_animTask.get());
    return a->GetScale();
  }

  return 1.0;
}

graphics::DisplayList * PinClickManager::GetSearchPinDL()
{
  using namespace graphics;

  if (!m_searchPinDL)
  {
    Screen * cacheScreen = m_f.GetGuiController()->GetCacheScreen();
    m_searchPinDL = cacheScreen->createDisplayList();

    cacheScreen->beginFrame();
    cacheScreen->setDisplayList(m_searchPinDL);

    Icon::Info infoKey("search-result-active");
    Resource const * res = cacheScreen->fromID(cacheScreen->findInfo(infoKey));
    shared_ptr<gl::BaseTexture> texture = cacheScreen->pipeline(res->m_pipelineID).texture();

    m2::RectU texRect = res->m_texRect;
    double sizeX = texRect.SizeX();
    double halfSizeX = sizeX / 2.0;
    double sizeY = texRect.SizeY();

    m2::PointD coords[] =
    {
      m2::PointD(-halfSizeX, -sizeY),
      m2::PointD(-halfSizeX, 0.0),
      m2::PointD(halfSizeX, -sizeY),
      m2::PointD(halfSizeX, 0.0)
    };
    m2::PointF normal(0.0, 0.0);

    m2::PointF texCoords[] =
    {
      texture->mapPixel(m2::PointF(texRect.minX(), texRect.minY())),
      texture->mapPixel(m2::PointF(texRect.minX(), texRect.maxY())),
      texture->mapPixel(m2::PointF(texRect.maxX(), texRect.minY())),
      texture->mapPixel(m2::PointF(texRect.maxX(), texRect.maxY()))
    };

    cacheScreen->addTexturedStripStrided(coords, sizeof(m2::PointD),
                                         &normal, 0,
                                         texCoords, sizeof(m2::PointF),
                                         4, graphics::activePinDepth, res->m_pipelineID);

    cacheScreen->setDisplayList(NULL);
    cacheScreen->endFrame();
  }

  return m_searchPinDL;
}

void PinClickManager::OnPositionClicked(m2::PointD const & pt)
{
  m_positionListener(pt.x, pt.y);
  m_updateForLocation = true;
}

void PinClickManager::Hide()
{
  m_updateForLocation = false;
  m_f.Invalidate();
}

void PinClickManager::OnClick(m2::PointD const & pxPoint, bool isLongTouch)
{
  // API
  url_scheme::ResultPoint apiPoint;
  if (m_f.GetMapApiPoint(pxPoint, apiPoint))
  {
    // @todo draw pin here
    OnActivateAPI(apiPoint);
    return;
  }

  // Everything else
  search::AddressInfo addrInfo;
  m2::PointD          pxPivot;
  BookmarkAndCategory bmAndCat;

  // By default we assume that we have pin
  m_hasPin = true;
  bool dispatched = false;

  switch (m_f.GetBookmarkOrPoi(pxPoint, pxPivot, addrInfo, bmAndCat))
  {
    case Framework::BOOKMARK:
    {
      OnActivateBookmark(bmAndCat);
      m_pinGlobalLocation = m_f.GetBmCategory(bmAndCat.first)->GetBookmark(bmAndCat.second)->GetOrg();
      dispatched = true;
      break;
    }

    case Framework::ADDTIONAL_LAYER:
    {
      OnAdditonalLayer(bmAndCat.second);
      m_pinGlobalLocation = m_f.GetBookmarkManager().AdditionalPoiLayerGetBookmark(bmAndCat.second)->GetOrg();
      dispatched = true;
      break;
    }

    case Framework::POI:
    {
      m2::PointD globalPoint = m_f.PtoG(pxPivot);
      OnActivatePOI(globalPoint, addrInfo);
      m_pinGlobalLocation = globalPoint;
      dispatched = true;
      break;
    }

    default:
    {
      if (isLongTouch)
      {
        m2::PointD const glbPoint = m_f.PtoG(pxPoint);
        m_f.GetAddressInfoForGlobalPoint(glbPoint, addrInfo);
        OnActivatePOI(glbPoint, addrInfo);
        m_pinGlobalLocation = glbPoint;
        dispatched = true;
      }
    }
  }

  if (!dispatched)
  {
    OnDismiss();
    m_hasPin = false;
    ResetAnimation();
  }
  else
    StartAnimation();

  m_f.Invalidate();
}

void PinClickManager::DrawPin(const shared_ptr<PaintEvent> & e)
{
  if (m_hasPin)
  {
    Navigator const    & navigator = m_f.GetNavigator();
    m2::AnyRectD const & glbRect = navigator.Screen().GlobalRect();

    // @todo change pin picture
    if (glbRect.IsPointInside(m_pinGlobalLocation))
    {
      m2::PointD pxPoint = navigator.GtoP(m_pinGlobalLocation);
      pxPoint += m2::PointD(0.0, 4 * m_f.GetVisualScale());
      graphics::DisplayList * dl = GetSearchPinDL();

      double scale = GetCurrentPinScale();
      math::Matrix<double, 3, 3> m = math::Shift(
                                                 math::Scale(math::Identity<double, 3>(),
                                                             scale, scale),
                                                 pxPoint.x, pxPoint.y);

      e->drawer()->screen()->drawDisplayList(dl, m);
    }
  }
}

void PinClickManager::RemovePin()
{
  ResetAnimation();
  m_hasPin = false;
  m_f.Invalidate();
}

void PinClickManager::Dismiss()
{
  OnDismiss();
}

void PinClickManager::ClearListeners()
{
  m_poiListener.clear();
  m_bookmarkListener.clear();
  m_apiListener.clear();
  m_positionListener.clear();
  m_additionalLayerListener.clear();
  m_dismissListener.clear();
}

void PinClickManager::OnActivateMyPosition()
{
  m_positionListener(0,0);
}

void PinClickManager::OnActivatePOI(m2::PointD const & globalPoint, search::AddressInfo const & info)
{
  m_poiListener(globalPoint, info);
}

void PinClickManager::OnActivateAPI(url_scheme::ResultPoint const & apiPoint)
{
  m_apiListener(apiPoint.GetPoint());
}

void PinClickManager::OnActivateBookmark(BookmarkAndCategory const & bmAndCat)
{
  m_bookmarkListener(bmAndCat);
}

void PinClickManager::OnAdditonalLayer(size_t index)
{
  m_additionalLayerListener(index);
}

void PinClickManager::OnDismiss()
{
  m_dismissListener();
}
