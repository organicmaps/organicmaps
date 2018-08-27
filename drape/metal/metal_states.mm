#include "drape/metal/metal_states.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include <utility>

namespace dp
{
namespace metal
{
namespace
{
// Bytes 7-4: [back function][back stencil fail action][back depth fail action][back pass action]
// Bytes 3-0: [front function][front stencil fail action][front depth fail action][front pass action]
uint8_t constexpr kStencilBackFunctionByte = 7;
uint8_t constexpr kStencilBackFailActionByte = 6;
uint8_t constexpr kStencilBackDepthFailActionByte = 5;
uint8_t constexpr kStencilBackPassActionByte = 4;
uint8_t constexpr kStencilFrontFunctionByte = 3;
uint8_t constexpr kStencilFrontFailActionByte = 2;
uint8_t constexpr kStencilFrontDepthFailActionByte = 1;
uint8_t constexpr kStencilFrontPassActionByte = 0;
  
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
  
  NSError * error = NULL;
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

void MetalStates::DepthStencilKey::SetStencilByte(uint8_t value, uint8_t byteNumber)
{
  auto const shift = byteNumber * 8;
  uint64_t const mask = ~(static_cast<uint64_t>(0xFF) << shift);
  m_stencil = (m_stencil & mask) | (static_cast<uint64_t>(value) << shift);
}
  
uint8_t MetalStates::DepthStencilKey::GetStencilByte(uint8_t byteNumber) const
{
  return static_cast<uint8_t>((m_stencil >> byteNumber * 8) & 0xFF);
}
  
void MetalStates::DepthStencilKey::SetStencilFunction(StencilFace face, TestFunction stencilFunction)
{
  switch (face)
  {
  case StencilFace::Front:
    SetStencilByte(static_cast<uint8_t>(stencilFunction), kStencilFrontFunctionByte);
    break;
  case StencilFace::Back:
    SetStencilByte(static_cast<uint8_t>(stencilFunction), kStencilBackFunctionByte);
    break;
  case StencilFace::FrontAndBack:
    SetStencilByte(static_cast<uint8_t>(stencilFunction), kStencilFrontFunctionByte);
    SetStencilByte(static_cast<uint8_t>(stencilFunction), kStencilBackFunctionByte);
    break;
  }
}
  
void MetalStates::DepthStencilKey::SetStencilActions(StencilFace face, StencilAction stencilFailAction,
                                                     StencilAction depthFailAction, StencilAction passAction)
{
  switch (face)
  {
  case StencilFace::Front:
    SetStencilByte(static_cast<uint8_t>(stencilFailAction), kStencilFrontFailActionByte);
    SetStencilByte(static_cast<uint8_t>(depthFailAction), kStencilFrontDepthFailActionByte);
    SetStencilByte(static_cast<uint8_t>(passAction), kStencilFrontPassActionByte);
    break;
  case StencilFace::Back:
    SetStencilByte(static_cast<uint8_t>(stencilFailAction), kStencilBackFailActionByte);
    SetStencilByte(static_cast<uint8_t>(depthFailAction), kStencilBackDepthFailActionByte);
    SetStencilByte(static_cast<uint8_t>(passAction), kStencilBackPassActionByte);
    break;
  case StencilFace::FrontAndBack:
    SetStencilByte(static_cast<uint8_t>(stencilFailAction), kStencilFrontFailActionByte);
    SetStencilByte(static_cast<uint8_t>(depthFailAction), kStencilFrontDepthFailActionByte);
    SetStencilByte(static_cast<uint8_t>(passAction), kStencilFrontPassActionByte);
    SetStencilByte(static_cast<uint8_t>(stencilFailAction), kStencilBackFailActionByte);
    SetStencilByte(static_cast<uint8_t>(depthFailAction), kStencilBackDepthFailActionByte);
    SetStencilByte(static_cast<uint8_t>(passAction), kStencilBackPassActionByte);
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
    frontDesc.stencilCompareFunction = DecodeTestFunction(GetStencilByte(kStencilFrontFunctionByte));
    frontDesc.stencilFailureOperation = DecodeStencilAction(GetStencilByte(kStencilFrontFailActionByte));
    frontDesc.depthFailureOperation = DecodeStencilAction(GetStencilByte(kStencilFrontDepthFailActionByte));
    frontDesc.depthStencilPassOperation = DecodeStencilAction(GetStencilByte(kStencilFrontPassActionByte));
    desc.frontFaceStencil = frontDesc;
    
    MTLStencilDescriptor * backDesc = [[MTLStencilDescriptor alloc] init];
    backDesc.stencilCompareFunction = DecodeTestFunction(GetStencilByte(kStencilBackFunctionByte));
    backDesc.stencilFailureOperation = DecodeStencilAction(GetStencilByte(kStencilBackFailActionByte));
    backDesc.depthFailureOperation = DecodeStencilAction(GetStencilByte(kStencilBackDepthFailActionByte));
    backDesc.depthStencilPassOperation = DecodeStencilAction(GetStencilByte(kStencilBackPassActionByte));
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
  ref_ptr<MetalGpuProgram> metalProgram = m_program;
  desc.vertexFunction = metalProgram->GetVertexShader();
  desc.fragmentFunction = metalProgram->GetFragmentShader();
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
}  // namespace metal
}  // namespace dp
