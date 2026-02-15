#pragma once

#include <string>

namespace dp
{
class GpuProgram
{
public:
  explicit GpuProgram(std::string_view programName) : m_programName(programName) {}

  virtual ~GpuProgram() = default;

  std::string_view GetName() const { return m_programName; }

  virtual void Bind() = 0;
  virtual void Unbind() = 0;

protected:
  std::string_view const m_programName;
};
}  // namespace dp
