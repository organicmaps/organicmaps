#pragma once

#include "gpu_drawer.hpp"

#include "graphics/color.hpp"
#include "graphics/resource_manager.hpp"
#include "graphics/screen.hpp"

#include "geometry/rect2d.hpp"

#include "std/function.hpp"
#include "std/shared_ptr.hpp"
#include "std/vector.hpp"


class PaintEvent;
class ScreenBase;
class VideoTimer;

namespace anim
{
  class Controller;
}

namespace graphics
{
  class RenderContext;

  class Skin;
  class GlyphCache;
  class ResourceManager;
  class Overlay;
}

namespace anim
{
  class Controller;
  class Task;
}

class WindowHandle;

// Base class for temporary CpuCount initialization work-around.
// m_cpuCoresCount should be initialized before RenderPolicy() constructor is called.
// New graphics library (Drape) does not have this issue.
class RenderPolicyCpuCountHolder
{
protected:
  RenderPolicyCpuCountHolder();
  // Cache this value as it can change dynamically at runtime.
  unsigned const m_cpuCoresCount;
};

class RenderPolicy
{
public:

  typedef function<void(shared_ptr<PaintEvent>,
                        ScreenBase const &,
                        m2::RectD const &,
                        int)> TRenderFn;

protected:

  vector<graphics::Color> m_bgColors;
  shared_ptr<graphics::ResourceManager> m_resourceManager;
  shared_ptr<graphics::Skin> m_skin;
  shared_ptr<graphics::Screen> m_cacheScreen;
  shared_ptr<graphics::RenderContext> m_primaryRC;
  shared_ptr<WindowHandle> m_windowHandle;
  shared_ptr<GPUDrawer> m_drawer;
  TRenderFn m_renderFn;
  bool m_doForceUpdate;
  m2::AnyRectD m_invalidRect;
  graphics::EDensity m_density;
  double m_visualScale;
  string m_skinName;
  anim::Controller * m_controller;

  void InitCacheScreen();

public:

  struct Params
  {
    VideoTimer * m_videoTimer;
    bool m_useDefaultFB;
    graphics::ResourceManager::Params m_rmParams;
    shared_ptr<graphics::RenderContext> m_primaryRC;
    graphics::EDensity m_density;
    string m_skinName;
    size_t m_screenWidth;
    size_t m_screenHeight;
  };

  /// constructor
  RenderPolicy(Params const & p,
               size_t idCacheSize);
  /// destructor
  virtual ~RenderPolicy();
  /// starting frame
  virtual void BeginFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);
  /// drawing single frame
  virtual void DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s) = 0;
  /// ending frame
  virtual void EndFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s);

  /// processing resize request
  virtual void OnSize(int w, int h);

  /// reacting on navigation actions
  /// @{
  virtual void StartDrag();
  virtual void DoDrag();
  virtual void StopDrag();

  virtual void StartScale();
  virtual void DoScale();
  virtual void StopScale();

  virtual void StartRotate(double a, double timeInSec);
  virtual void DoRotate(double a, double timeInSec);
  virtual void StopRotate(double a, double timeInSec);
  /// @}

  /// the start point of rendering in renderpolicy.
  virtual void SetRenderFn(TRenderFn const & renderFn);

  void SetAnimController(anim::Controller * controller);

  virtual bool IsTiling() const;

  virtual bool NeedRedraw() const;
  virtual bool IsEmptyModel() const;

  bool DoForceUpdate() const;
  void SetForceUpdate(bool flag);

  bool IsAnimating() const;

  void SetInvalidRect(m2::AnyRectD const & glbRect);
  m2::AnyRectD const & GetInvalidRect() const;

  shared_ptr<GPUDrawer> const & GetDrawer() const;
  shared_ptr<WindowHandle> const & GetWindowHandle() const;
  graphics::GlyphCache * GetGlyphCache() const;

  double VisualScale() const;
  virtual size_t TileSize() const { return 256; }
  graphics::EDensity Density() const;
  string const & SkinName() const;

  /// This function is used when we need to prevent race
  /// conditions on some resources, which could be modified
  /// from another threads.
  /// One example of such resource is a current graphics::Overlay
  /// object
  /// @{
  virtual void FrameLock();
  virtual void FrameUnlock();
  /// @}

  /// Get current graphics::Overlay object.
  /// Access to this resource should be synchronized using
  /// FrameLock/FrameUnlock methods
  virtual graphics::Overlay * FrameOverlay() const;

  /// Benchmarking protocol
  virtual int InsertBenchmarkFence();
  virtual void JoinBenchmarkFence(int fenceID);

  shared_ptr<graphics::Screen> const & GetCacheScreen() const;
  shared_ptr<graphics::ResourceManager> const & GetResourceManager() const { return m_resourceManager; }
  shared_ptr<graphics::RenderContext> const & GetRenderContext() const { return m_primaryRC; }

  graphics::Screen * CreateScreenWithParams(graphics::Screen::Params const & params) const;

protected:
  void InitWindowsHandle(VideoTimer * timer, shared_ptr<graphics::RenderContext> context);
  GPUDrawer * CreateDrawer(bool isDefaultFB,
                           shared_ptr<graphics::RenderContext> context,
                           graphics::EStorageType storageType,
                           graphics::ETextureType textureType);

  size_t GetLargeTextureSize(bool useNpot);
  size_t GetMediumTextureSize(bool useNpot);
  size_t GetSmallTextureSize(bool useNpot);

  graphics::ResourceManager::StoragePoolParams GetStorageParam(size_t vertexCount,
                                                               size_t indexCount,
                                                               size_t batchSize,
                                                               graphics::EStorageType type);
  graphics::ResourceManager::TexturePoolParams GetTextureParam(size_t size,
                                                               size_t initCount,
                                                               graphics::DataFormat format,
                                                               graphics::ETextureType type);
};

RenderPolicy * CreateRenderPolicy(RenderPolicy::Params const & params);

graphics::GlyphCache::Params GetGlyphCacheParams(graphics::EDensity density, size_t cacheMaxSize = 2 * 1024 * 1024);
graphics::ResourceManager::GlyphCacheParams GetResourceGlyphCacheParams(graphics::EDensity density, size_t cacheMaxSize = 2 * 1024 * 1024);
