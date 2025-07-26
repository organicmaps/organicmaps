#pragma once

#import <MetalKit/MetalKit.h>

#include "drape/color.hpp"
#include "drape/glsl_types.hpp"
#include "drape/pointers.hpp"

namespace dp
{
class GpuProgram;

namespace metal
{
class MetalBaseContext;

class MetalCleaner
{
public:
  MetalCleaner() = default;

  void Init(ref_ptr<MetalBaseContext> context, drape_ptr<GpuProgram> && programClearColor,
            drape_ptr<GpuProgram> && programClearDepth, drape_ptr<GpuProgram> && programClearColorAndDepth);

  void SetClearColor(Color const & color);

  void ClearDepth(ref_ptr<MetalBaseContext> context, id<MTLRenderCommandEncoder> encoder);
  void ClearColor(ref_ptr<MetalBaseContext> context, id<MTLRenderCommandEncoder> encoder);
  void ClearColorAndDepth(ref_ptr<MetalBaseContext> context, id<MTLRenderCommandEncoder> encoder);

private:
  void ApplyColorParam(id<MTLRenderCommandEncoder> encoder, ref_ptr<GpuProgram> program);
  void RenderQuad(ref_ptr<MetalBaseContext> context, id<MTLRenderCommandEncoder> encoder, ref_ptr<GpuProgram> program);

  id<MTLBuffer> m_buffer;
  id<MTLDepthStencilState> m_depthEnabledState;
  id<MTLDepthStencilState> m_depthDisabledState;
  glsl::vec4 m_clearColor;

  drape_ptr<GpuProgram> m_programClearColor;
  drape_ptr<GpuProgram> m_programClearDepth;
  drape_ptr<GpuProgram> m_programClearColorAndDepth;
};
}  // namespace metal
}  // namespace dp
