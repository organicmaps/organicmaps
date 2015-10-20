#pragma once

#include "viewport.hpp"

#include "drape/gpu_program_manager.hpp"

#include "base/matrix.hpp"

namespace df
{

class Renderer3d
{
public:
  Renderer3d();
  ~Renderer3d();

  void SetSize(uint32_t width, uint32_t height);
  void SetVerticalFOV(float fov);
  void SetPlaneAngleX(float angleX);

  math::Matrix<float, 4, 4> const & GetTransform() const;

  float GetScaleX() const;
  float GetScaleY() const;

  void Render(uint32_t textureId, ref_ptr<dp::GpuProgramManager> mng);

private:
  void Build(ref_ptr<dp::GpuProgram> prg);

  void CalculateGeometry();

  void UpdateScaleMatrix();
  void UpdateRotationMatrix();
  void UpdateTranslationMatrix();
  void UpdateProjectionMatrix();

  uint32_t m_width;
  uint32_t m_height;

  float m_fov;
  float m_angleX;
  float m_offsetY;
  float m_offsetZ;

  float m_scaleX;
  float m_scaleY;

  math::Matrix<float, 4, 4> m_scaleMatrix;
  math::Matrix<float, 4, 4> m_rotationMatrix;
  math::Matrix<float, 4, 4> m_translationMatrix;
  math::Matrix<float, 4, 4> m_projectionMatrix;
  math::Matrix<float, 4, 4> m_transformMatrix;

  uint32_t m_VAO;
  uint32_t m_bufferId;

  array<float, 16> m_vertices;
};

}
