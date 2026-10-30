#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "drape/batcher.hpp"
#include "drape/framebuffer.hpp"
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
///   fixture.Render("Test Name", 400, 300, [](ShapeTestFixture & f)
///   {
///     f.AddShape(make_unique_dp<LineShape>(spline, params));
///   });
///   QImage const & img = fixture.GetLastImage();
///
class ShapeTestFixture
{
public:
  using ShapeCreatorFn = std::function<void(ShapeTestFixture &)>;

  /// Create shapes, render them through the GPU pipeline, and store the result as QImage.
  /// Handles GL context lifecycle (RunTestInOpenGLOffscreenEnvironment).
  /// @param createShapes — functor that calls AddShape() to populate geometry.
  void Render(char const * title, uint32_t width, uint32_t height, ShapeCreatorFn const & createShapes);

  /// Add a shape to the current batch. Only valid inside the Render() callback.
  void AddShape(drape_ptr<MapShape> && shape);

  /// Cache user lines into the current batch. Only valid inside the Render() callback.
  void AddUserLines(TileKey const & tileKey, TracksSource const & source,
                    UserLinesRenderCollection const & renderParams);
  void AddUserAreas(TileKey const & tileKey, UserAreasToCache const & areas);

  /// Sets the scale from world coordinates to pixels. Must be called before Render().
  void SetWorldToPixelScale(float scale) { m_worldToPixelScale = scale; }

  /// The last rendered frame; valid after Render() returns.
  QImage const & GetLastImage() const { return m_lastImage; }

private:
  void Init(uint32_t width, uint32_t height);
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
  float m_worldToPixelScale = 1.0f;
  drape_ptr<dp::Framebuffer> m_framebuffer;

  std::unique_ptr<dp::GraphicsContext> m_context;
  std::unique_ptr<dp::TextureManager> m_texMng;
  std::unique_ptr<gpu::ProgramManager> m_progMng;
  std::unique_ptr<dp::Batcher> m_batcher;

  std::vector<BucketEntry> m_buckets;
  QImage m_lastImage;
};
}  // namespace df::test_support
