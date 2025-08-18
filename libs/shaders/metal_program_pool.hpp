#pragma once
#import <MetalKit/MetalKit.h>

#include "shaders/program_pool.hpp"

#include "drape/pointers.hpp"

#include <map>
#include <string>

namespace gpu
{
enum class SystemProgram
{
  ClearColor = 0,
  ClearDepth,
  ClearColorAndDepth,

  SystemProgramsCount
};

namespace metal
{
class MetalProgramPool : public ProgramPool
{
public:
  explicit MetalProgramPool(id<MTLDevice> device);
  ~MetalProgramPool() override;

  drape_ptr<dp::GpuProgram> Get(Program program) override;
  drape_ptr<dp::GpuProgram> GetSystemProgram(SystemProgram program);

private:
  drape_ptr<dp::GpuProgram> Get(std::string const & programName, std::string const & vertexShaderName,
                                std::string const & fragmentShaderName, std::map<uint8_t, uint8_t> const & layout);

  id<MTLFunction> GetFunction(std::string const & name);
  id<MTLDevice> m_device;
  id<MTLLibrary> m_library;
  std::map<std::string, id<MTLFunction>> m_functions;
};
}  // namespace metal
}  // namespace gpu
