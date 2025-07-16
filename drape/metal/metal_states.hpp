#pragma once
#import <MetalKit/MetalKit.h>

#include "drape/graphics_context.hpp"
#include "drape/metal/metal_gpu_program.hpp"
#include "drape/pointers.hpp"
#include "drape/texture_types.hpp"

#include <cstdint>
#include <map>

namespace dp
{
namespace metal
{
class MetalStates
{
public:
  struct DepthStencilKey
  {
    void SetDepthTestEnabled(bool enabled);
    void SetDepthTestFunction(TestFunction depthFunction);
    void SetStencilTestEnabled(bool enabled);
    void SetStencilFunction(StencilFace face, TestFunction stencilFunction);
    void SetStencilActions(StencilFace face, StencilAction stencilFailAction, StencilAction depthFailAction,
                           StencilAction passAction);
    bool operator<(DepthStencilKey const & rhs) const;
    MTLDepthStencilDescriptor * BuildDescriptor() const;

    bool m_depthEnabled = false;
    bool m_stencilEnabled = false;
    TestFunction m_depthFunction = TestFunction::Always;
    uint64_t m_stencil = 0;
  };

  struct PipelineKey
  {
    PipelineKey() = default;
    PipelineKey(ref_ptr<GpuProgram> program, MTLPixelFormat colorFormat, MTLPixelFormat depthStencilFormat,
                bool blendingEnabled);

    bool operator<(PipelineKey const & rhs) const;
    MTLRenderPipelineDescriptor * BuildDescriptor() const;

    ref_ptr<GpuProgram> m_program;
    MTLPixelFormat m_colorFormat = MTLPixelFormatInvalid;
    MTLPixelFormat m_depthStencilFormat = MTLPixelFormatInvalid;
    bool m_blendingEnabled = false;
  };

  struct SamplerKey
  {
    SamplerKey() = default;
    SamplerKey(TextureFilter filter, TextureWrapping wrapSMode, TextureWrapping wrapTMode);
    void Set(TextureFilter filter, TextureWrapping wrapSMode, TextureWrapping wrapTMode);
    bool operator<(SamplerKey const & rhs) const;
    MTLSamplerDescriptor * BuildDescriptor() const;

    uint32_t m_sampler = 0;
  };

  id<MTLDepthStencilState> GetDepthStencilState(id<MTLDevice> device, DepthStencilKey const & key);
  id<MTLRenderPipelineState> GetPipelineState(id<MTLDevice> device, PipelineKey const & key);
  id<MTLSamplerState> GetSamplerState(id<MTLDevice> device, SamplerKey const & key);

  void ResetPipelineStatesCache();

private:
  using DepthStencilCache = std::map<DepthStencilKey, id<MTLDepthStencilState>>;
  DepthStencilCache m_depthStencilCache;

  using PipelineCache = std::map<PipelineKey, id<MTLRenderPipelineState>>;
  PipelineCache m_pipelineCache;

  using SamplerCache = std::map<SamplerKey, id<MTLSamplerState>>;
  SamplerCache m_samplerCache;
};
}  // namespace metal
}  // namespace dp
