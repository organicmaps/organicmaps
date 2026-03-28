#pragma once

#include "drape_frontend/map_shape.hpp"

#include "drape/batcher.hpp"
#include "drape/render_bucket.hpp"
#include "drape/render_state.hpp"
#include "drape/texture_manager.hpp"

#include "shaders/program_manager.hpp"

#include <QtGui/QImage>

#include <memory>
#include <vector>

namespace df::test_support
{
/// GPU-based testing fixture for MapShape subclasses.
/// Creates a real OpenGL context, compiles shaders, initializes textures,
/// and renders shapes to an offscreen framebuffer.
///
/// Usage:
///   ShapeTestFixture fixture;
///   fixture.Init(400, 300);
///   fixture.AddShape(make_unique_dp<LineShape>(spline, params));
///   fixture.Flush();
///   QImage img = fixture.Render();
///   fixture.ShowInWindow("Test Name");
///
class ShapeTestFixture
{
public:
  ShapeTestFixture();
  ~ShapeTestFixture();

  /// Initialize the GL context, texture manager, program manager, and FBO.
  void Init(uint32_t width, uint32_t height);

  /// Add a shape to the current batch. Call Flush() after adding all shapes.
  void AddShape(drape_ptr<MapShape> && shape);

  /// Finalize the batcher — produces RenderBuckets from all added shapes.
  void Flush();

  /// Render all flushed buckets to the FBO and return the result as a QImage.
  QImage Render();

  /// Display the rendered image in a Qt window (blocks until window is closed).
  void ShowInWindow(char const * title, bool autoExit = false);

  /// Access the texture manager (e.g. to set up test colors).
  ref_ptr<dp::TextureManager> GetTextureManager() { return make_ref(m_texMng); }

private:
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
