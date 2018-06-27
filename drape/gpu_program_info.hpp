#pragma once

#include <cstdint>
#include <string>

namespace gpu
{
struct GpuProgramInfo
{
  GpuProgramInfo() = default;
  GpuProgramInfo(int vertexIndex, int fragmentIndex, char const * vertexSource,
                 char const * fragmentSource, uint8_t textureSlotsCount)
    : m_vertexIndex(vertexIndex)
    , m_fragmentIndex(fragmentIndex)
    , m_vertexSource(vertexSource)
    , m_fragmentSource(fragmentSource)
    , m_textureSlotsCount(textureSlotsCount)
  {}
  int m_vertexIndex = -1;
  int m_fragmentIndex = -1;
  char const * m_vertexSource = nullptr;
  char const * m_fragmentSource = nullptr;
  uint8_t m_textureSlotsCount = 0;
};

class GpuProgramGetter
{
public:
  virtual ~GpuProgramGetter() = default;
  virtual gpu::GpuProgramInfo const & GetProgramInfo(int program) const = 0;
};
}  // namespace gpu
