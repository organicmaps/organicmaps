#include "metal_window.hpp"

#include "iphone/Maps/Core/MapRendering/MetalContextFactory.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <algorithm>
#include <cstdint>

namespace qt::common::renderer::metal
{
MetalWindow::MetalWindow(Framework & framework, QWindow * parent)
  : RendererWindow(framework, MetalSurface, QSurfaceFormat(), parent)
{}

MetalWindow::~MetalWindow() {
  m_framework.EnterBackground();
  m_framework.SetRenderingDisabled(true);

  if (m_contextFactory != nullptr)
  {
    m_framework.DestroyDrapeEngine();
    m_contextFactory.reset();
  }
}

void MetalWindow::Render()
{
  if (!isExposed())
    return;

  EnsureInitialized();
}

void MetalWindow::EnsureInitialized()
{
  if (m_contextFactory != nullptr)
    return;

  if (winId() == 0)
    create();

  NSView * view = (__bridge NSView *)(reinterpret_cast<void *>(static_cast<uintptr_t>(winId())));
  CHECK(view != nil, ());

  CAMetalLayer * layer = nil;
  if ([view.layer isKindOfClass:[CAMetalLayer class]])
    layer = static_cast<CAMetalLayer *>(view.layer);
  else
  {
    layer = [CAMetalLayer layer];
    view.wantsLayer = YES;
    view.layer = layer;
  }

  id<MTLDevice> const device = MTLCreateSystemDefaultDevice();
  CHECK(device != nil, ());

  m_ratio = devicePixelRatio();
  layer.device = device;
  layer.opaque = YES;
  layer.displaySyncEnabled = YES;
  layer.contentsScale = m_ratio;

  auto const pixelWidth = static_cast<uint32_t>(std::max(1, static_cast<int>(m_ratio * width())));
  auto const pixelHeight = static_cast<uint32_t>(std::max(1, static_cast<int>(m_ratio * height())));
  layer.drawableSize = CGSizeMake(pixelWidth, pixelHeight);

  m_contextFactory = make_unique_dp<MetalContextFactory>(layer, m2::PointU{pixelWidth, pixelHeight});

  CreateDrapeEngine(dp::ApiVersion::Metal, make_ref(m_contextFactory));
  m_framework.EnterForeground();

  m_contextFactory->WaitForInitialization(nullptr);
  OnResize(width(), height());
}
}  // namespace qt::common::renderer::metal
