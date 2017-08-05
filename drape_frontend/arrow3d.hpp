#pragma once

#include "drape_frontend/render_state.hpp"

#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"

#include "std/vector.hpp"

namespace dp
{
class GpuProgram;
class GpuProgramManager;
class TextureManager;
}

class ScreenBase;

namespace df
{

class Arrow3d
{
public:
  Arrow3d();
  ~Arrow3d();

  void SetPosition(m2::PointD const & position);
  void SetAzimuth(double azimuth);
  void SetTexture(ref_ptr<dp::TextureManager> texMng);
  void SetPositionObsolete(bool obsolete);

  void Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
              bool routingMode);

private:
  void Build();
  math::Matrix<float, 4, 4> CalculateTransform(ScreenBase const & screen,
                                               float dz, float scaleFactor) const;
  void RenderArrow(ScreenBase const & screen, ref_ptr<dp::GpuProgram> program,
                   dp::Color const & color, float dz,
                   float scaleFactor, bool hasNormals);

  m2::PointD m_position;
  double m_azimuth = 0.0;
  bool m_obsoletePosition = false;

  uint32_t m_VAO = 0;
  uint32_t m_bufferId = 0;
  uint32_t m_bufferNormalsId = 0;

  vector<float> m_vertices;
  vector<float> m_normals;

  dp::GLState m_state;

  bool m_isInitialized = false;
};

}  // namespace df

