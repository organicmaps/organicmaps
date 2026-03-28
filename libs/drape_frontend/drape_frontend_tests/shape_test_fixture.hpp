#pragma once

#include "drape_frontend/map_shape.hpp"

#include "drape/batcher.hpp"
#include "drape/render_bucket.hpp"
#include "drape/render_state.hpp"
#include "drape/texture_manager.hpp"

#include "shaders/program_manager.hpp"

#include <QtGui/QImage>

#include <functional>
#include <memory>
#include <vector>

namespace df::test_support
{
/// GPU-based test fixture for MapShape subclasses.
/// Renders shapes through the real OpenGL shader pipeline to an offscreen FBO.
///
/// Usage:
///   ShapeTestFixture fixture;
///   fixture.RenderShapesToImage(400, 300, [](ShapeTestFixture & f)
///   {
///     f.AddShape(make_unique_dp<LineShape>(spline, params));
///   });
///   fixture.ShowInWindow("Test Name");
///
class ShapeTestFixture
{
public:
  using ShapeCreatorFn = std::function<void(ShapeTestFixture &)>;

  /// Create shapes, render them through the GPU pipeline, and store the result as QImage.
  /// Handles GL context lifecycle (RunTestInOpenGLOffscreenEnvironment).
  /// @param createShapes — functor that calls AddShape() to populate geometry.
  void Render(char const * title, uint32_t width, uint32_t height, ShapeCreatorFn const & createShapes);

  /// Add a shape to the current batch. Only valid inside RenderShapesToImage callback.
  void AddShape(drape_ptr<MapShape> && shape);

private:
  bool Init(uint32_t width, uint32_t height);
  void Flush();
  void Render();
  void ReleaseGLResources();

  struct BucketEntry
  {
    dp::RenderState m_state;
    drape_ptr<dp::RenderBucket> m_bucket;
  };

  uint32_t m_width = 0;
  uint32_t m_height = 0;
  uint32_t m_fbo = 0;
  uint32_t m_colorRbo = 0;
  uint32_t m_depthRbo = 0;

  std::unique_ptr<dp::GraphicsContext> m_context;
  std::unique_ptr<dp::TextureManager> m_texMng;
  std::unique_ptr<gpu::ProgramManager> m_progMng;
  std::unique_ptr<dp::Batcher> m_batcher;

  std::vector<BucketEntry> m_buckets;
  QImage m_lastImage;
};
}  // namespace df::test_support
