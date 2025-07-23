#pragma once

#include <cstdint>
#include <string>

namespace gpu
{
struct GLProgramInfo
{
  GLProgramInfo() = default;
  GLProgramInfo(std::string const & vertexShaderName, std::string const & fragmentShaderName,
                char const * const vertexSource, char const * const fragmentSource)
    : m_vertexShaderName(vertexShaderName)
    , m_fragmentShaderName(fragmentShaderName)
    , m_vertexShaderSource(vertexSource)
    , m_fragmentShaderSource(fragmentSource)
  {}

  std::string const m_vertexShaderName;
  std::string const m_fragmentShaderName;
  char const * const m_vertexShaderSource = nullptr;
  char const * const m_fragmentShaderSource = nullptr;
};
}  // namespace gpu
