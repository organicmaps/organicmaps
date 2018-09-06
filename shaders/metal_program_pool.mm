#include "shaders/metal_program_pool.hpp"
#include "shaders/program_params.hpp"

#include "drape/metal/metal_gpu_program.hpp"

#include "base/assert.hpp"

#include <utility>

namespace gpu
{
namespace metal
{
namespace
{
struct ProgramInfo
{
  std::string m_vertexShaderName;
  std::string m_fragmentShaderName;
  ProgramInfo(std::string && vertexShaderName, std::string && fragmentShaderName)
    : m_vertexShaderName(std::move(vertexShaderName))
    , m_fragmentShaderName(std::move(fragmentShaderName))
  {}
};

std::array<ProgramInfo, static_cast<size_t>(Program::ProgramsCount)> const kMetalProgramsInfo = {{
  ProgramInfo("", ""),  // ColoredSymbol
  ProgramInfo("", ""),  // Texturing
  ProgramInfo("", ""),  // MaskedTexturing
  ProgramInfo("", ""),  // Bookmark
  ProgramInfo("", ""),  // BookmarkAnim
  ProgramInfo("", ""),  // TextOutlined
  ProgramInfo("", ""),  // Text
  ProgramInfo("", ""),  // TextFixed
  ProgramInfo("", ""),  // TextOutlinedGui
  ProgramInfo("", ""),  // Area
  ProgramInfo("", ""),  // AreaOutline
  ProgramInfo("", ""),  // Area3d
  ProgramInfo("", ""),  // Area3dOutline
  ProgramInfo("", ""),  // Line
  ProgramInfo("", ""),  // CapJoin
  ProgramInfo("", ""),  // TransitCircle
  ProgramInfo("", ""),  // DashedLine
  ProgramInfo("", ""),  // PathSymbol
  ProgramInfo("", ""),  // HatchingArea
  ProgramInfo("", ""),  // TexturingGui
  ProgramInfo("", ""),  // Ruler
  ProgramInfo("", ""),  // Accuracy
  ProgramInfo("", ""),  // MyPosition
  ProgramInfo("", ""),  // Transit
  ProgramInfo("", ""),  // TransitMarker
  ProgramInfo("", ""),  // Route
  ProgramInfo("", ""),  // RouteDash
  ProgramInfo("", ""),  // RouteArrow
  ProgramInfo("", ""),  // RouteMarker
  ProgramInfo("", ""),  // CirclePoint
  ProgramInfo("vsDebugRect", "fsDebugRect"),  // DebugRect
  ProgramInfo("vsScreenQuad", "fsScreenQuad"),  // ScreenQuad
  ProgramInfo("", ""),  // Arrow3d
  ProgramInfo("", ""),  // Arrow3dShadow
  ProgramInfo("", ""),  // Arrow3dOutline
  ProgramInfo("", ""),  // ColoredSymbolBillboard
  ProgramInfo("", ""),  // TexturingBillboard
  ProgramInfo("", ""),  // MaskedTexturingBillboard
  ProgramInfo("", ""),  // BookmarkBillboard
  ProgramInfo("", ""),  // BookmarkAnimBillboard
  ProgramInfo("", ""),  // TextOutlinedBillboard
  ProgramInfo("", ""),  // TextBillboard
  ProgramInfo("", ""),  // TextFixedBillboard
  ProgramInfo("", ""),  // Traffic
  ProgramInfo("", ""),  // TrafficLine
  ProgramInfo("", ""),  // TrafficCircle
  ProgramInfo("", ""),  // SmaaEdges
  ProgramInfo("", ""),  // SmaaBlendingWeight
  ProgramInfo("", ""),  // SmaaFinal
}};
}  // namespace

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

drape_ptr<dp::GpuProgram> MetalProgramPool::Get(Program program)
{
  auto const & info = kMetalProgramsInfo[static_cast<size_t>(program)];
  CHECK(!info.m_vertexShaderName.empty(), ());
  CHECK(!info.m_fragmentShaderName.empty(), ());

  id<MTLFunction> vertexShader = GetFunction(info.m_vertexShaderName);
  id<MTLFunction> fragmentShader = GetFunction(info.m_fragmentShaderName);
  
  MTLRenderPipelineDescriptor * desc = [[MTLRenderPipelineDescriptor alloc] init];
  desc.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
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
  
  // Uniforms buffer must have the name "uniforms".
  NSString * kUniformsName = @"uniforms";
  
  int8_t vsUniformsBindingIndex = dp::metal::MetalGpuProgram::kInvalidBindingIndex;
  for (MTLArgument * arg in reflectionObj.vertexArguments)
  {
    if ([arg.name compare:kUniformsName] == NSOrderedSame && arg.active && arg.type == MTLArgumentTypeBuffer)
    {
      vsUniformsBindingIndex = static_cast<int8_t>(arg.index);
      break;
    }
  }
  
  int8_t fsUniformsBindingIndex = dp::metal::MetalGpuProgram::kInvalidBindingIndex;
  dp::metal::MetalGpuProgram::TexturesBindingInfo textureBindingInfo;
  for (MTLArgument * arg in reflectionObj.fragmentArguments)
  {
    if ([arg.name compare:kUniformsName] == NSOrderedSame && arg.active && arg.type == MTLArgumentTypeBuffer)
    {
      fsUniformsBindingIndex = static_cast<int8_t>(arg.index);
    }
    else if (arg.type == MTLArgumentTypeTexture)
    {
      std::string const name([arg.name UTF8String]);
      textureBindingInfo[name].m_textureBindingIndex = static_cast<int8_t>(arg.index);
    }
    else if (arg.type == MTLArgumentTypeSampler)
    {
      std::string const name([arg.name UTF8String]);
      // Sampler name must be constructed as concatenation of texture name and kSamplerSuffix.
      static std::string const kSamplerSuffix = "Sampler";
      auto const pos = name.find(kSamplerSuffix);
      if (pos == std::string::npos)
        continue;
      std::string const textureName = name.substr(0, pos);
      textureBindingInfo[textureName].m_samplerBindingIndex = static_cast<int8_t>(arg.index);
    }
  }
  
  auto const name = DebugPrint(program);
  return make_unique_dp<dp::metal::MetalGpuProgram>(name, vertexShader, fragmentShader,
                                                    vsUniformsBindingIndex, fsUniformsBindingIndex,
                                                    std::move(textureBindingInfo));
}
  
id<MTLFunction> MetalProgramPool::GetFunction(std::string const & name)
{
  auto const it = m_functions.find(name);
  if (it == m_functions.end())
  {
    id<MTLFunction> f = [m_library newFunctionWithName:@(name.c_str())];
    CHECK(f != nil, ());
    f.label = [@"Function " stringByAppendingString:@(name.c_str())];
    m_functions.insert(std::make_pair(name, f));
    return f;
  }
  return it->second;
}
}  // namespace metal
}  // namespace gpu
