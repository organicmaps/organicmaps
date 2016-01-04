#pragma once

#include "drape/glstate.hpp"
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
  void SetSize(uint32_t width, uint32_t height);
  void SetTexture(ref_ptr<dp::TextureManager> texMng);

  void Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng);

private:
  void Build(ref_ptr<dp::GpuProgram> prg);

  m2::PointD m_position;
  double m_azimuth;

  uint32_t m_pixelWidth = 0;
  uint32_t m_pixelHeight = 0;

  uint32_t m_bufferId = 0;
  uint32_t m_bufferNormalsId = 0;

  int8_t m_attributePosition;
  int8_t m_attributeNormal;

  vector<float> m_vertices;
  vector<float> m_normals;

  dp::GLState m_state;

  bool m_isInitialized = false;
};

}  // namespace df

