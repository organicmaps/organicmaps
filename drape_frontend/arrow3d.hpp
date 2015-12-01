#pragma once

#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"

#include "std/vector.hpp"

namespace dp
{
class GpuProgram;
class GpuProgramManager;
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

  void Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng);

private:
  void Build(ref_ptr<dp::GpuProgram> prg);

  m2::PointD m_position;
  double m_azimuth;

  uint32_t m_pixelWidth = 0;
  uint32_t m_pixelHeight = 0;

  uint32_t m_VAO = 0;
  uint32_t m_bufferId = 0;
  uint32_t m_bufferNormalsId = 0;

  vector<float> m_vertices;
  vector<float> m_normals;
};

}  // namespace df

