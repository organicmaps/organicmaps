#pragma once

#include <string>

namespace dp
{
class GpuProgram
{
public:
  explicit GpuProgram(std::string const & programName) : m_programName(programName) {}

  virtual ~GpuProgram() = default;

  std::string const & GetName() const { return m_programName; }

  virtual void Bind() = 0;
  virtual void Unbind() = 0;

protected:
  std::string const m_programName;
};
}  // namespace dp
