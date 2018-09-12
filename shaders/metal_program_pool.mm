#include "shaders/metal_program_pool.hpp"
#include "shaders/program_params.hpp"

#include "drape/metal/metal_gpu_program.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace gpu
{
namespace metal
{
namespace
{
struct ProgramInfo
{
  using Layout = std::map<uint8_t, uint8_t>;
  std::string const m_vertexShaderName;
  std::string const m_fragmentShaderName;
  Layout m_layout;
  
  // Layout is in the format { buffer0, buffer1, ..., bufferN }.
  // bufferX is a pair { start attribute index, end attribute index }.
  ProgramInfo(std::string && vertexShaderName, std::string && fragmentShaderName,
              std::vector<std::pair<uint8_t, uint8_t>> const & layout)
    : m_vertexShaderName(std::move(vertexShaderName))
    , m_fragmentShaderName(std::move(fragmentShaderName))
  {
    for (size_t i = 0; i < layout.size(); i++)
    {
      for (size_t j = layout[i].first; j <= layout[i].second; j++)
      {
        CHECK(m_layout.find(j) == m_layout.end(), ("Duplicate index in the layout."));
        m_layout[j] = i;
      }
    }
  }
};
  
std::array<ProgramInfo, static_cast<size_t>(SystemProgram::SystemProgramsCount)> const kMetalSystemProgramsInfo = {{
  ProgramInfo("vsCleaner", "fsClearColor", {{0, 0}}),  // ClearColor
  ProgramInfo("vsCleaner", "fsClearDepth", {{0, 0}}),  // ClearDepth
  ProgramInfo("vsCleaner", "fsClearColorAndDepth", {{0, 0}}),  // ClearColorAndDepth
}};
  
std::array<ProgramInfo, static_cast<size_t>(Program::ProgramsCount)> const kMetalProgramsInfo = {{
  ProgramInfo("", "", {}),  // ColoredSymbol
  ProgramInfo("", "", {}),  // Texturing
  ProgramInfo("", "", {}),  // MaskedTexturing
  ProgramInfo("", "", {}),  // Bookmark
  ProgramInfo("", "", {}),  // BookmarkAnim
  ProgramInfo("", "", {}),  // TextOutlined
  ProgramInfo("", "", {}),  // Text
  ProgramInfo("", "", {}),  // TextFixed
  ProgramInfo("vsTextStaticOutlinedGui", "fsTextOutlinedGui", {{0, 4}}),  // TextStaticOutlinedGui
  ProgramInfo("vsTextOutlinedGui", "fsTextOutlinedGui", {{0, 2}, {3, 4}}),  // TextOutlinedGui
  ProgramInfo("", "", {}),  // Area
  ProgramInfo("", "", {}),  // AreaOutline
  ProgramInfo("", "", {}),  // Area3d
  ProgramInfo("", "", {}),  // Area3dOutline
  ProgramInfo("", "", {}),  // Line
  ProgramInfo("", "", {}),  // CapJoin
  ProgramInfo("", "", {}),  // TransitCircle
  ProgramInfo("", "", {}),  // DashedLine
  ProgramInfo("", "", {}),  // PathSymbol
  ProgramInfo("", "", {}),  // HatchingArea
  ProgramInfo("vsTexturingGui", "fsTexturingGui", {{0, 1}}),  // TexturingGui
  ProgramInfo("vsRuler", "fsRuler", {{0, 2}}),  // Ruler
  ProgramInfo("", "", {}),  // Accuracy
  ProgramInfo("", "", {}),  // MyPosition
  ProgramInfo("", "", {}),  // Transit
  ProgramInfo("", "", {}),  // TransitMarker
  ProgramInfo("", "", {}),  // Route
  ProgramInfo("", "", {}),  // RouteDash
  ProgramInfo("", "", {}),  // RouteArrow
  ProgramInfo("", "", {}),  // RouteMarker
  ProgramInfo("", "", {}),  // CirclePoint
  ProgramInfo("vsDebugRect", "fsDebugRect", {{0, 0}}),  // DebugRect
  ProgramInfo("vsScreenQuad", "fsScreenQuad", {{0, 1}}),  // ScreenQuad
  ProgramInfo("", "", {}),  // Arrow3d
  ProgramInfo("", "", {}),  // Arrow3dShadow
  ProgramInfo("", "", {}),  // Arrow3dOutline
  ProgramInfo("", "", {}),  // ColoredSymbolBillboard
  ProgramInfo("", "", {}),  // TexturingBillboard
  ProgramInfo("", "", {}),  // MaskedTexturingBillboard
  ProgramInfo("", "", {}),  // BookmarkBillboard
  ProgramInfo("", "", {}),  // BookmarkAnimBillboard
  ProgramInfo("", "", {}),  // TextOutlinedBillboard
  ProgramInfo("", "", {}),  // TextBillboard
  ProgramInfo("", "", {}),  // TextFixedBillboard
  ProgramInfo("", "", {}),  // Traffic
  ProgramInfo("", "", {}),  // TrafficLine
  ProgramInfo("", "", {}),  // TrafficCircle
  ProgramInfo("", "", {}),  // SmaaEdges
  ProgramInfo("", "", {}),  // SmaaBlendingWeight
  ProgramInfo("", "", {}),  // SmaaFinal
}};
  
MTLVertexFormat GetFormatByDataType(MTLDataType dataType)
{
  switch (dataType)
  {
  case MTLDataTypeFloat: return MTLVertexFormatFloat;
  case MTLDataTypeFloat2: return MTLVertexFormatFloat2;
  case MTLDataTypeFloat3: return MTLVertexFormatFloat3;
  case MTLDataTypeFloat4: return MTLVertexFormatFloat4;
  }
  CHECK(false, ("Unsupported vertex format."));
  return MTLVertexFormatInvalid;
}
  
uint32_t GetSizeByDataType(MTLDataType dataType)
{
  switch (dataType)
  {
  case MTLDataTypeFloat: return sizeof(float);
  case MTLDataTypeFloat2: return 2 * sizeof(float);
  case MTLDataTypeFloat3: return 3 * sizeof(float);
  case MTLDataTypeFloat4: return 4 * sizeof(float);
  }
  CHECK(false, ("Unsupported vertex format."));
  return 0;
}
  
void GetBindings(NSArray<MTLArgument *> * arguments, int8_t & uniformsBindingIndex,
                 dp::metal::MetalGpuProgram::TexturesBindingInfo & textureBindingInfo)
{
  // Uniforms buffer must have the name "uniforms".
  NSString * kUniformsName = @"uniforms";
  // Sampler name must be constructed as concatenation of texture name and kSamplerSuffix.
  static std::string const kSamplerSuffix = "Sampler";
  
  uniformsBindingIndex = dp::metal::MetalGpuProgram::kInvalidBindingIndex;

  for (MTLArgument * arg in arguments)
  {
    if ([arg.name compare:kUniformsName] == NSOrderedSame && arg.active && arg.type == MTLArgumentTypeBuffer)
    {
      uniformsBindingIndex = static_cast<int8_t>(arg.index);
    }
    else if (arg.type == MTLArgumentTypeTexture)
    {
      std::string const name([arg.name UTF8String]);
      textureBindingInfo[name].m_textureBindingIndex = static_cast<int8_t>(arg.index);
    }
    else if (arg.type == MTLArgumentTypeSampler)
    {
      std::string const name([arg.name UTF8String]);
      auto const pos = name.find(kSamplerSuffix);
      if (pos == std::string::npos)
        continue;
      std::string const textureName = name.substr(0, pos);
      textureBindingInfo[textureName].m_samplerBindingIndex = static_cast<int8_t>(arg.index);
    }
  }
}
  
MTLVertexDescriptor * GetVertexDescriptor(id<MTLFunction> vertexShader, ProgramInfo::Layout const & layout)
{
  MTLVertexDescriptor * vertexDesc = [[MTLVertexDescriptor alloc] init];
  uint32_t offset = 0;
  uint32_t lastBufferIndex = 0;
  std::map<uint8_t, uint32_t> sizes;
  for (MTLVertexAttribute * attr in vertexShader.vertexAttributes)
  {
    auto const attrIndex = static_cast<uint8_t>(attr.attributeIndex);
    auto const it = layout.find(attrIndex);
    CHECK(it != layout.cend(), ("Invalid layout."));
    auto const bufferIndex = it->second;
    if (lastBufferIndex != bufferIndex)
    {
      offset = 0;
      lastBufferIndex = bufferIndex;
    }
    MTLVertexAttributeDescriptor * attrDesc = vertexDesc.attributes[attr.attributeIndex];
    attrDesc.format = GetFormatByDataType(attr.attributeType);
    auto const sz = GetSizeByDataType(attr.attributeType);
    attrDesc.offset = offset;
    offset += sz;
    sizes[bufferIndex] += sz;
    attrDesc.bufferIndex = bufferIndex;
  }
  
  for (auto const & s : sizes)
    vertexDesc.layouts[s.first].stride = s.second;
  
  return vertexDesc;
}
}  // namespace

std::string DebugPrint(SystemProgram p)
{
  switch (p)
  {
  case SystemProgram::ClearColor: return "ClearColor";
  case SystemProgram::ClearDepth: return "ClearDepth";
  case SystemProgram::ClearColorAndDepth: return "ClearColorAndDepth";
    
  case SystemProgram::SystemProgramsCount:
    CHECK(false, ("Try to output SystemProgramsCount"));
  }
  CHECK(false, ("Unknown program"));
  return {};
}
  
MetalProgramPool::MetalProgramPool(id<MTLDevice> device)
  : m_device(device)
{
  ProgramParams::Init();
  
  NSString * libPath = [[NSBundle mainBundle] pathForResource:@"shaders_metal" ofType:@"metallib"];
  NSError * error = nil;
  m_library = [m_device newLibraryWithFile:libPath error:&error];
  if (error)
  {
    NSLog(@"%@", error);
    CHECK(false, ("Shaders library creation error."));
  }
  m_library.label = @"Shaders library";
}

MetalProgramPool::~MetalProgramPool()
{
  ProgramParams::Destroy();
}

drape_ptr<dp::GpuProgram> MetalProgramPool::GetSystemProgram(SystemProgram program)
{
  auto const & info = kMetalSystemProgramsInfo[static_cast<size_t>(program)];
  return Get(DebugPrint(program), info.m_vertexShaderName, info.m_fragmentShaderName, info.m_layout);
}
  
drape_ptr<dp::GpuProgram> MetalProgramPool::Get(Program program)
{
  auto const & info = kMetalProgramsInfo[static_cast<size_t>(program)];
  return Get(DebugPrint(program), info.m_vertexShaderName, info.m_fragmentShaderName, info.m_layout);
}
  
drape_ptr<dp::GpuProgram> MetalProgramPool::Get(std::string const & programName,
                                                std::string const & vertexShaderName,
                                                std::string const & fragmentShaderName,
                                                std::map<uint8_t, uint8_t> const & layout)
{
  CHECK(!vertexShaderName.empty(), ());
  CHECK(!fragmentShaderName.empty(), ());
  
  id<MTLFunction> vertexShader = GetFunction(vertexShaderName);
  id<MTLFunction> fragmentShader = GetFunction(fragmentShaderName);
  MTLVertexDescriptor * vertexDesc = GetVertexDescriptor(vertexShader, layout);

  // Reflect functions.
  MTLRenderPipelineDescriptor * desc = [[MTLRenderPipelineDescriptor alloc] init];
  desc.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
  desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
  desc.vertexDescriptor = vertexDesc;
  desc.vertexFunction = vertexShader;
  desc.fragmentFunction = fragmentShader;
  
  NSError * error = nil;
  MTLRenderPipelineReflection * reflectionObj = nil;
  MTLPipelineOption option = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;
  id<MTLRenderPipelineState> pso = [m_device newRenderPipelineStateWithDescriptor:desc
                                                                          options:option
                                                                       reflection:&reflectionObj
                                                                            error:&error];
  if (error != nil || pso == nil)
  {
    NSLog(@"%@", error);
    CHECK(false, ("Failed to create reflection pipeline state."));
  }
  
  int8_t vsUniformsBindingIndex = dp::metal::MetalGpuProgram::kInvalidBindingIndex;
  dp::metal::MetalGpuProgram::TexturesBindingInfo vsTextureBindingInfo;
  GetBindings(reflectionObj.vertexArguments, vsUniformsBindingIndex, vsTextureBindingInfo);
  
  int8_t fsUniformsBindingIndex = dp::metal::MetalGpuProgram::kInvalidBindingIndex;
  dp::metal::MetalGpuProgram::TexturesBindingInfo fsTextureBindingInfo;
  GetBindings(reflectionObj.fragmentArguments, fsUniformsBindingIndex, fsTextureBindingInfo);
  
  return make_unique_dp<dp::metal::MetalGpuProgram>(programName, vertexShader, fragmentShader,
                                                    vsUniformsBindingIndex, fsUniformsBindingIndex,
                                                    std::move(vsTextureBindingInfo),
                                                    std::move(fsTextureBindingInfo),
                                                    vertexDesc);
}
  
id<MTLFunction> MetalProgramPool::GetFunction(std::string const & name)
{
  auto const it = m_functions.find(name);
  if (it == m_functions.end())
  {
    id<MTLFunction> f = [m_library newFunctionWithName:@(name.c_str())];
    CHECK(f != nil, ());
    if (@available(iOS 10.0, *))
      f.label = [@"Function " stringByAppendingString:@(name.c_str())];
    m_functions.insert(std::make_pair(name, f));
    return f;
  }
  return it->second;
}
}  // namespace metal
}  // namespace gpu
