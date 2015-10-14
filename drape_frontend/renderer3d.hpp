#pragma once

#include "drape/gpu_program_manager.hpp"
#include "viewport.hpp"

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

  float GetScaleX() const;
  float GetScaleY() const;

  void Render(uint32_t textureId, ref_ptr<dp::GpuProgramManager> mng);

private:
  void Build(ref_ptr<dp::GpuProgram> prg);

  void CalculateGeometry();

  void UpdateRotationMatrix();
  void UpdateTranslationMatrix();
  void UpdateProjectionMatrix();

  uint32_t m_width;
  uint32_t m_height;

  float m_fov;
  float m_angleX;
  float m_offsetX;
  float m_offsetY;
  float m_offsetZ;

  float m_scaleX;
  float m_scaleY;

  array<float, 16> m_rotationMatrix;
  array<float, 16> m_translationMatrix;
  array<float, 16> m_projectionMatrix;

  uint32_t m_VAO;
  uint32_t m_bufferId;

  float m_vertices[16];
};

}
