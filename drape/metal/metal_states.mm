#include "drape/metal/metal_states.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace dp
{
namespace metal
{
namespace
{
// Stencil package.
uint8_t constexpr kStencilBackFunctionByte = 7;
uint8_t constexpr kStencilBackFailActionByte = 6;
uint8_t constexpr kStencilBackDepthFailActionByte = 5;
uint8_t constexpr kStencilBackPassActionByte = 4;
uint8_t constexpr kStencilFrontFunctionByte = 3;
uint8_t constexpr kStencilFrontFailActionByte = 2;
uint8_t constexpr kStencilFrontDepthFailActionByte = 1;
uint8_t constexpr kStencilFrontPassActionByte = 0;

// Sampler package.
uint8_t constexpr kWrapSModeByte = 3;
uint8_t constexpr kWrapTModeByte = 2;
uint8_t constexpr kMagFilterByte = 1;
uint8_t constexpr kMinFilterByte = 0;

template<typename T>
void SetStateByte(T & state, uint8_t value, uint8_t byteNumber)
{
  auto const shift = byteNumber * 8;
  auto const mask = ~(static_cast<T>(0xFF) << shift);
  state = (state & mask) | (static_cast<T>(value) << shift);
}

template<typename T>
uint8_t GetStateByte(T & state, uint8_t byteNumber)
{
  return static_cast<uint8_t>((state >> byteNumber * 8) & 0xFF);
}
  
MTLCompareFunction DecodeTestFunction(uint8_t testFunctionByte)
{
  switch (static_cast<TestFunction>(testFunctionByte))
  {
  case TestFunction::Never: return MTLCompareFunctionNever;
  case TestFunction::Less: return MTLCompareFunctionLess;
  case TestFunction::Equal: return MTLCompareFunctionEqual;
  case TestFunction::LessOrEqual: return MTLCompareFunctionLessEqual;
  case TestFunction::Greater: return MTLCompareFunctionGreater;
  case TestFunction::NotEqual: return MTLCompareFunctionNotEqual;
  case TestFunction::GreaterOrEqual: return MTLCompareFunctionGreaterEqual;
  case TestFunction::Always: return MTLCompareFunctionAlways;
  }
  ASSERT(false, ());
}
  
MTLStencilOperation DecodeStencilAction(uint8_t stencilActionByte)
{
  switch (static_cast<StencilAction>(stencilActionByte))
  {
  case StencilAction::Keep: return MTLStencilOperationKeep;
  case StencilAction::Zero: return MTLStencilOperationZero;
  case StencilAction::Replace: return MTLStencilOperationReplace;
  case StencilAction::Increment: return MTLStencilOperationIncrementClamp;
  case StencilAction::IncrementWrap: return MTLStencilOperationIncrementWrap;
  case StencilAction::Decrement: return MTLStencilOperationDecrementClamp;
  case StencilAction::DecrementWrap: return MTLStencilOperationDecrementWrap;
  case StencilAction::Invert: return MTLStencilOperationInvert;
  }
  ASSERT(false, ());
}
  
MTLSamplerMinMagFilter DecodeTextureFilter(uint8_t textureFilterByte)
{
  switch (static_cast<TextureFilter>(textureFilterByte))
  {
  case TextureFilter::Nearest: return MTLSamplerMinMagFilterNearest;
  case TextureFilter::Linear: return MTLSamplerMinMagFilterLinear;
  }
  ASSERT(false, ());
}
  
MTLSamplerAddressMode DecodeTextureWrapping(uint8_t textureWrappingByte)
{
  switch (static_cast<TextureWrapping>(textureWrappingByte))
  {
  case TextureWrapping::ClampToEdge: return MTLSamplerAddressModeClampToEdge;
  case TextureWrapping::Repeat: return MTLSamplerAddressModeRepeat;
  }
  ASSERT(false, ());
}
  
bool IsStencilFormat(MTLPixelFormat format)
{
  return format == MTLPixelFormatDepth32Float_Stencil8 ||
         format == MTLPixelFormatStencil8 ||
         format == MTLPixelFormatX32_Stencil8;
}
}  // namespace

id<MTLDepthStencilState> MetalStates::GetDepthStencilState(id<MTLDevice> device, DepthStencilKey const & key)
{
  auto const it = m_depthStencilCache.find(key);
  if (it != m_depthStencilCache.end())
    return it->second;
  
  id<MTLDepthStencilState> depthState = [device newDepthStencilStateWithDescriptor:key.BuildDescriptor()];
  CHECK(depthState != nil, ());
  m_depthStencilCache.insert(std::make_pair(key, depthState));
  return depthState;
}
  
id<MTLRenderPipelineState> MetalStates::GetPipelineState(id<MTLDevice> device, PipelineKey const & key)
{
  auto const it = m_pipelineCache.find(key);
  if (it != m_pipelineCache.end())
    return it->second;
  
  NSError * error = nil;
  id<MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:key.BuildDescriptor()
                                                                                    error:&error];
  if (pipelineState == nil || error != nil)
  {
    NSLog(@"%@", error);
    CHECK(false, ("Failed to create pipeline state."));
  }
  m_pipelineCache.insert(std::make_pair(key, pipelineState));
  return pipelineState;
}

id<MTLSamplerState> MetalStates::GetSamplerState(id<MTLDevice> device, SamplerKey const & key)
{
  auto const it = m_samplerCache.find(key);
  if (it != m_samplerCache.end())
    return it->second;
  
  id<MTLSamplerState> samplerState = [device newSamplerStateWithDescriptor:key.BuildDescriptor()];
  CHECK(samplerState != nil, ());
  m_samplerCache.insert(std::make_pair(key, samplerState));
  return samplerState;
}
  
void MetalStates::ResetPipelineStatesCache()
{
  m_pipelineCache.clear();
}

void MetalStates::DepthStencilKey::SetDepthTestEnabled(bool enabled)
{
  m_depthEnabled = enabled;
}
  
void MetalStates::DepthStencilKey::SetDepthTestFunction(TestFunction depthFunction)
{
  m_depthFunction = depthFunction;
}
  
void MetalStates::DepthStencilKey::SetStencilTestEnabled(bool enabled)
{
  m_stencilEnabled = enabled;
}
  
void MetalStates::DepthStencilKey::SetStencilFunction(StencilFace face, TestFunction stencilFunction)
{
  switch (face)
  {
  case StencilFace::Front:
    SetStateByte(m_stencil, static_cast<uint8_t>(stencilFunction), kStencilFrontFunctionByte);
    break;
  case StencilFace::Back:
    SetStateByte(m_stencil, static_cast<uint8_t>(stencilFunction), kStencilBackFunctionByte);
    break;
  case StencilFace::FrontAndBack:
    SetStateByte(m_stencil, static_cast<uint8_t>(stencilFunction), kStencilFrontFunctionByte);
    SetStateByte(m_stencil, static_cast<uint8_t>(stencilFunction), kStencilBackFunctionByte);
    break;
  }
}
  
void MetalStates::DepthStencilKey::SetStencilActions(StencilFace face, StencilAction stencilFailAction,
                                                     StencilAction depthFailAction, StencilAction passAction)
{
  switch (face)
  {
  case StencilFace::Front:
    SetStateByte(m_stencil, static_cast<uint8_t>(stencilFailAction), kStencilFrontFailActionByte);
    SetStateByte(m_stencil, static_cast<uint8_t>(depthFailAction), kStencilFrontDepthFailActionByte);
    SetStateByte(m_stencil, static_cast<uint8_t>(passAction), kStencilFrontPassActionByte);
    break;
  case StencilFace::Back:
    SetStateByte(m_stencil, static_cast<uint8_t>(stencilFailAction), kStencilBackFailActionByte);
    SetStateByte(m_stencil, static_cast<uint8_t>(depthFailAction), kStencilBackDepthFailActionByte);
    SetStateByte(m_stencil, static_cast<uint8_t>(passAction), kStencilBackPassActionByte);
    break;
  case StencilFace::FrontAndBack:
    SetStateByte(m_stencil, static_cast<uint8_t>(stencilFailAction), kStencilFrontFailActionByte);
    SetStateByte(m_stencil, static_cast<uint8_t>(depthFailAction), kStencilFrontDepthFailActionByte);
    SetStateByte(m_stencil, static_cast<uint8_t>(passAction), kStencilFrontPassActionByte);
    SetStateByte(m_stencil, static_cast<uint8_t>(stencilFailAction), kStencilBackFailActionByte);
    SetStateByte(m_stencil, static_cast<uint8_t>(depthFailAction), kStencilBackDepthFailActionByte);
    SetStateByte(m_stencil, static_cast<uint8_t>(passAction), kStencilBackPassActionByte);
    break;
  }
}

bool MetalStates::DepthStencilKey::operator<(DepthStencilKey const & rhs) const
{
  if (m_depthEnabled != rhs.m_depthEnabled)
    return m_depthEnabled < rhs.m_depthEnabled;
  
  if (m_stencilEnabled != rhs.m_stencilEnabled)
    return m_stencilEnabled < rhs.m_stencilEnabled;
  
  if (m_depthFunction != rhs.m_depthFunction)
    return m_depthFunction < rhs.m_depthFunction;
  
  return m_stencil < rhs.m_stencil;
}

MTLDepthStencilDescriptor * MetalStates::DepthStencilKey::BuildDescriptor() const
{
  MTLDepthStencilDescriptor * desc = [[MTLDepthStencilDescriptor alloc] init];
  if (m_depthEnabled)
  {
    desc.depthWriteEnabled = YES;
    desc.depthCompareFunction = DecodeTestFunction(static_cast<uint8_t>(m_depthFunction));
  }
  else
  {
    desc.depthWriteEnabled = NO;
    desc.depthCompareFunction = MTLCompareFunctionAlways;
  }
  if (m_stencilEnabled)
  {
    MTLStencilDescriptor * frontDesc = [[MTLStencilDescriptor alloc] init];
    frontDesc.stencilCompareFunction = DecodeTestFunction(GetStateByte(m_stencil, kStencilFrontFunctionByte));
    frontDesc.stencilFailureOperation = DecodeStencilAction(GetStateByte(m_stencil, kStencilFrontFailActionByte));
    frontDesc.depthFailureOperation = DecodeStencilAction(GetStateByte(m_stencil, kStencilFrontDepthFailActionByte));
    frontDesc.depthStencilPassOperation = DecodeStencilAction(GetStateByte(m_stencil, kStencilFrontPassActionByte));
    desc.frontFaceStencil = frontDesc;
    
    MTLStencilDescriptor * backDesc = [[MTLStencilDescriptor alloc] init];
    backDesc.stencilCompareFunction = DecodeTestFunction(GetStateByte(m_stencil, kStencilBackFunctionByte));
    backDesc.stencilFailureOperation = DecodeStencilAction(GetStateByte(m_stencil, kStencilBackFailActionByte));
    backDesc.depthFailureOperation = DecodeStencilAction(GetStateByte(m_stencil, kStencilBackDepthFailActionByte));
    backDesc.depthStencilPassOperation = DecodeStencilAction(GetStateByte(m_stencil, kStencilBackPassActionByte));
    desc.backFaceStencil = backDesc;
  }
  else
  {
    desc.frontFaceStencil = nil;
    desc.backFaceStencil = nil;
  }
  return desc;
}

MetalStates::PipelineKey::PipelineKey(ref_ptr<GpuProgram> program, MTLPixelFormat colorFormat,
                                      MTLPixelFormat depthStencilFormat, bool blendingEnabled)
  : m_program(std::move(program))
  , m_colorFormat(colorFormat)
  , m_depthStencilFormat(depthStencilFormat)
  , m_blendingEnabled(blendingEnabled)
{}
  
bool MetalStates::PipelineKey::operator<(PipelineKey const & rhs) const
{
  if (m_program != rhs.m_program)
    return m_program < rhs.m_program;
  
  if (m_colorFormat != rhs.m_colorFormat)
    return m_colorFormat < rhs.m_colorFormat;
  
  if (m_depthStencilFormat != rhs.m_depthStencilFormat)
    return m_depthStencilFormat < rhs.m_depthStencilFormat;
  
  return m_blendingEnabled < rhs.m_blendingEnabled;
}
  
MTLRenderPipelineDescriptor * MetalStates::PipelineKey::BuildDescriptor() const
{
  MTLRenderPipelineDescriptor * desc = [[MTLRenderPipelineDescriptor alloc] init];
  desc.rasterSampleCount = 1;
  desc.vertexBuffers[0].mutability = MTLMutabilityImmutable;  // The first VB is always immutable.
  ref_ptr<MetalGpuProgram> metalProgram = m_program;
  desc.vertexFunction = metalProgram->GetVertexShader();
  desc.fragmentFunction = metalProgram->GetFragmentShader();
  desc.vertexDescriptor = metalProgram->GetVertexDescriptor();
  MTLRenderPipelineColorAttachmentDescriptor * colorAttachment = desc.colorAttachments[0];
  colorAttachment.pixelFormat = m_colorFormat;
  desc.depthAttachmentPixelFormat = m_depthStencilFormat;
  if (IsStencilFormat(m_depthStencilFormat))
    desc.stencilAttachmentPixelFormat = m_depthStencilFormat;
  else
    desc.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
  colorAttachment.blendingEnabled = m_blendingEnabled ? YES : NO;
  colorAttachment.rgbBlendOperation = MTLBlendOperationAdd;
  colorAttachment.alphaBlendOperation = MTLBlendOperationAdd;
  colorAttachment.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
  colorAttachment.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
  colorAttachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
  colorAttachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
  return desc;
}
  
MetalStates::SamplerKey::SamplerKey(TextureFilter filter, TextureWrapping wrapSMode, TextureWrapping wrapTMode)
{
  Set(filter, wrapSMode, wrapTMode);
}
  
void MetalStates::SamplerKey::Set(TextureFilter filter, TextureWrapping wrapSMode, TextureWrapping wrapTMode)
{
  SetStateByte(m_sampler, static_cast<uint8_t>(filter), kMinFilterByte);
  SetStateByte(m_sampler, static_cast<uint8_t>(filter), kMagFilterByte);
  SetStateByte(m_sampler, static_cast<uint8_t>(wrapSMode), kWrapSModeByte);
  SetStateByte(m_sampler, static_cast<uint8_t>(wrapTMode), kWrapTModeByte);
}
  
bool MetalStates::SamplerKey::operator<(SamplerKey const & rhs) const
{
  return m_sampler < rhs.m_sampler;
}
  
MTLSamplerDescriptor * MetalStates::SamplerKey::BuildDescriptor() const
{
  MTLSamplerDescriptor * desc = [[MTLSamplerDescriptor alloc] init];
  desc.minFilter = DecodeTextureFilter(GetStateByte(m_sampler, kMinFilterByte));
  desc.magFilter = DecodeTextureFilter(GetStateByte(m_sampler, kMagFilterByte));
  desc.sAddressMode = DecodeTextureWrapping(GetStateByte(m_sampler, kWrapSModeByte));
  desc.tAddressMode = DecodeTextureWrapping(GetStateByte(m_sampler, kWrapTModeByte));
  return desc;
}
}  // namespace metal
}  // namespace dp
