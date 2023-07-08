#include "drape/metal/metal_cleaner.hpp"
#include "drape/metal/metal_base_context.hpp"
#include "drape/metal/metal_gpu_program.hpp"

#include <vector>

namespace dp
{
namespace metal
{
void MetalCleaner::Init(ref_ptr<MetalBaseContext> context,
                        drape_ptr<GpuProgram> && programClearColor,
                        drape_ptr<GpuProgram> && programClearDepth,
                        drape_ptr<GpuProgram> && programClearColorAndDepth)
{
  m_programClearColor = std::move(programClearColor);
  m_programClearDepth = std::move(programClearDepth);
  m_programClearColorAndDepth = std::move(programClearColorAndDepth);
  
  ref_ptr<MetalBaseContext> metalContext = context;
  id<MTLDevice> device = metalContext->GetMetalDevice();
  std::vector<float> quad = {-1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f};
  m_buffer = [device newBufferWithBytes:quad.data()
                                 length:quad.size() * sizeof(quad[0])
                                options:MTLResourceCPUCacheModeWriteCombined];
  m_buffer.label = @"MetalCleaner";
  
  MTLDepthStencilDescriptor * desc = [[MTLDepthStencilDescriptor alloc] init];
  desc.depthWriteEnabled = YES;
  desc.depthCompareFunction = MTLCompareFunctionAlways;
  m_depthEnabledState = [device newDepthStencilStateWithDescriptor:desc];
  CHECK(m_depthEnabledState != nil, ());
  
  desc.depthWriteEnabled = NO;
  desc.depthCompareFunction = MTLCompareFunctionAlways;
  m_depthDisabledState = [device newDepthStencilStateWithDescriptor:desc];
  CHECK(m_depthDisabledState != nil, ());
}

void MetalCleaner::SetClearColor(Color const & color)
{
  m_clearColor = glsl::ToVec4(color);
}
  
void MetalCleaner::ApplyColorParam(id<MTLRenderCommandEncoder> encoder, ref_ptr<GpuProgram> program)
{
  ref_ptr<MetalGpuProgram> metalProgram = program;
  auto const fsBindingIndex = metalProgram->GetFragmentShaderUniformsBindingIndex();
  if (fsBindingIndex >= 0)
  {
    [encoder setFragmentBytes:(void const *)&m_clearColor length:sizeof(m_clearColor)
                      atIndex:fsBindingIndex];
  }
}
  
void MetalCleaner::RenderQuad(ref_ptr<MetalBaseContext> metalContext, id<MTLRenderCommandEncoder> encoder,
                              ref_ptr<GpuProgram> program)
{
  id<MTLRenderPipelineState> pipelineState = metalContext->GetPipelineState(program, false /* blendingEnabled */);
  if (pipelineState == nil)
    return;

  [encoder setRenderPipelineState:pipelineState];
  
  [encoder setVertexBuffer:m_buffer offset:0 atIndex:0];
  [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
}
  
void MetalCleaner::ClearDepth(ref_ptr<MetalBaseContext> context, id<MTLRenderCommandEncoder> encoder)
{
  [encoder pushDebugGroup:@"ClearDepth"];
  [encoder setDepthStencilState:m_depthEnabledState];
  RenderQuad(context, encoder, make_ref(m_programClearDepth));
  [encoder popDebugGroup];
}

void MetalCleaner::ClearColor(ref_ptr<MetalBaseContext> context, id<MTLRenderCommandEncoder> encoder)
{
  [encoder pushDebugGroup:@"ClearColor"];
  [encoder setDepthStencilState:m_depthDisabledState];
  ApplyColorParam(encoder, make_ref(m_programClearColor));
  RenderQuad(context, encoder, make_ref(m_programClearColor));
  [encoder popDebugGroup];
}

void MetalCleaner::ClearColorAndDepth(ref_ptr<MetalBaseContext> context, id<MTLRenderCommandEncoder> encoder)
{
  [encoder pushDebugGroup:@"ClearColorAndDepth"];
  [encoder setDepthStencilState:m_depthEnabledState];
  ApplyColorParam(encoder, make_ref(m_programClearColorAndDepth));
  RenderQuad(context, encoder, make_ref(m_programClearColorAndDepth));
  [encoder popDebugGroup];
}
}  // namespace metal
}  // namespace dp
