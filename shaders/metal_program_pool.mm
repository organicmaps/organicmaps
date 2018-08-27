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
  ProgramInfo(std::string && vertexShaderName, std::string fragmentShaderName)
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
{
  ProgramParams::Init();
  
  NSString * libPath = [[NSBundle mainBundle] pathForResource:@"shaders_metal" ofType:@"metallib"];
  NSError * error;
  m_library = [device newLibraryWithFile:libPath error:&error];
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

  auto const name = DebugPrint(program);
  return make_unique_dp<dp::metal::MetalGpuProgram>(name, GetFunction(info.m_vertexShaderName),
                                                    GetFunction(info.m_fragmentShaderName));
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
