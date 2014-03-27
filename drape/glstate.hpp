#pragma once

#include "pointers.hpp"
#include "uniform_values_storage.hpp"
#include "texture_set_controller.hpp"
#include "color.hpp"

class GLState
{
public:
  GLState(uint32_t gpuProgramIndex, int16_t depthLayer);

  void SetTextureSet(int32_t textureSet);
  int32_t GetTextureSet() const;
  bool HasTextureSet() const;

  void SetColor(Color const & c);
  Color const & GetColor() const;
  bool HasColor() const;

  int GetProgramIndex() const;

  bool operator<(const GLState & other) const;

private:
  uint32_t m_gpuProgramIndex;
  uint16_t m_depthLayer;
  int32_t m_textureSet;
  Color m_color;

  uint32_t m_mask;
};

void ApplyUniforms(const UniformValuesStorage & uniforms, RefPointer<GpuProgram> program);
void ApplyState(GLState state, RefPointer<GpuProgram> program, RefPointer<TextureSetController> textures);
