#pragma once
#import <MetalKit/MetalKit.h>

#include "shaders/program_pool.hpp"

#include "drape/pointers.hpp"

#include <map>
#include <string>

namespace gpu
{
namespace metal
{
class MetalProgramPool : public ProgramPool
{
public:
  explicit MetalProgramPool(id<MTLDevice> device);
  ~MetalProgramPool() override;

  drape_ptr<dp::GpuProgram> Get(Program program) override;

private:
  id<MTLFunction> GetFunction(std::string const & name);
  id<MTLDevice> m_device;
  id<MTLLibrary> m_library;
  std::map<std::string, id<MTLFunction>> m_functions;
};
}  // namespace metal
}  // namespace gpu
