#pragma once

#include "shaders/programs.hpp"

#include "drape/drape_global.hpp"
#include "drape/glsl_types.hpp"
#include "drape/gpu_program.hpp"
#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"

#include "base/assert.hpp"

#include "std/target_os.hpp"

#include <map>
#include <vector>

namespace gpu
{
class ProgramParams
{
public:
  static void Init();
  static void Destroy();
  static std::string GetBoundParamsName(ref_ptr<dp::GpuProgram> program);

private:
  static std::map<std::string, std::string> m_boundParams;
};

#define BIND_PROGRAMS(ParamsType, ...)                                                              \
  static std::vector<gpu::Program> const & GetBoundPrograms()                                       \
  {                                                                                                 \
    static std::vector<gpu::Program> programs = {__VA_ARGS__};                                      \
    return programs;                                                                                \
  }                                                                                                 \
  static std::string GetName()                                                                      \
  {                                                                                                 \
    return std::string(#ParamsType);                                                                \
  }                                                                                                 \
  static void BindPrograms(std::map<std::string, std::string> & params)                             \
  {                                                                                                 \
    for (auto const p : GetBoundPrograms())                                                         \
    {                                                                                               \
      auto const programName = DebugPrint(p);                                                       \
      CHECK(params.find(programName) == params.cend(), ("Program has already bound", programName)); \
      params[programName] = GetName();                                                              \
    }                                                                                               \
  }

#define ALIGNMENT alignas(16)

// NOTE: structs may contain dummy elements to fit MSL and GLSL struct alignment rules
// 1. Add new fields in order from the highest byte size to the lowest, it minimizes alignment overhead
// 2. Keep 16 bytes alignment for the whole struct, it complements the size of the latest element to vec4
// 3. Consider vec3 as vec4, add float complement and don't reuse it

struct ALIGNMENT MapProgramParams
{
  glsl::mat4 m_modelView;
  glsl::mat4 m_projection;
  glsl::mat4 m_pivotTransform;
  glsl::vec2 m_contrastGamma;
  float m_opacity = 1.0f;
  float m_zScale = 1.0f;
  float m_interpolation = 1.0f;
  float m_isOutlinePass = 1.0f;

  BIND_PROGRAMS(MapProgramParams, Program::Area, Program::Area3d, Program::Area3dOutline, Program::AreaOutline,
                Program::Bookmark, Program::BookmarkAnim, Program::BookmarkAnimBillboard, Program::BookmarkBillboard,
                Program::CapJoin, Program::CirclePoint, Program::ColoredSymbol, Program::ColoredSymbolBillboard,
                Program::DashedLine, Program::TransparentArea, Program::HatchingArea, Program::Line,
                Program::MaskedTexturing, Program::MaskedTexturingBillboard, Program::PathSymbol, Program::Text,
                Program::TextBillboard, Program::TextOutlined, Program::TextOutlinedBillboard, Program::Texturing,
                Program::TexturingBillboard, Program::BookmarkAboveText, Program::BookmarkAnimAboveText,
                Program::BookmarkAnimAboveTextBillboard, Program::BookmarkAboveTextBillboard)
};

struct ALIGNMENT RouteProgramParams
{
  glsl::mat4 m_modelView;
  glsl::mat4 m_projection;
  glsl::mat4 m_pivotTransform;
  glsl::vec4 m_routeParams;
  glsl::vec4 m_color;
  glsl::vec4 m_maskColor;
  glsl::vec4 m_outlineColor;
  glsl::vec4 m_fakeColor;
  glsl::vec4 m_fakeOutlineColor;
  glsl::vec2 m_fakeBorders;
  glsl::vec2 m_pattern;
  glsl::vec2 m_angleCosSin;
  float m_arrowHalfWidth = 0.0f;
  float m_opacity = 1.0f;

  BIND_PROGRAMS(RouteProgramParams, Program::Route, Program::RouteDash, Program::RouteArrow, Program::RouteMarker)
};

struct ALIGNMENT TrafficProgramParams
{
  glsl::mat4 m_modelView;
  glsl::mat4 m_projection;
  glsl::mat4 m_pivotTransform;
  glsl::vec4 m_trafficParams;
  union
  {
    glsl::vec4 m_outlineColorAligned;
    glsl::vec3 m_outlineColor;
  };
  union
  {
    glsl::vec4 m_lightArrowColorAligned;
    glsl::vec3 m_lightArrowColor;
  };
  union
  {
    glsl::vec4 m_darkArrowColorAligned;
    glsl::vec3 m_darkArrowColor;
  };
  float m_outline = 0.0f;
  float m_opacity = 1.0f;

  TrafficProgramParams()
    : m_outlineColorAligned(0.0f, 0.0f, 0.0f, 0.0f)
    , m_lightArrowColorAligned(0.0f, 0.0f, 0.0f, 0.0f)
    , m_darkArrowColorAligned(0.0f, 0.0f, 0.0f, 0.0f)
  {}

  BIND_PROGRAMS(TrafficProgramParams, Program::Traffic, Program::TrafficLine, Program::TrafficCircle)
};

struct ALIGNMENT TransitProgramParams
{
  glsl::mat4 m_modelView;
  glsl::mat4 m_projection;
  glsl::mat4 m_pivotTransform;
  union
  {
    glsl::vec4 m_paramsAligned;
    glsl::vec3 m_params;
  };
  float m_lineHalfWidth = 0.0f;
  float m_maxRadius = 0.0f;

  TransitProgramParams() : m_paramsAligned(0.0f, 0.0f, 0.0f, 0.0f) {}

  BIND_PROGRAMS(TransitProgramParams, Program::Transit, Program::TransitCircle, Program::TransitMarker)
};

struct ALIGNMENT GuiProgramParams
{
  glsl::mat4 m_modelView;
  glsl::mat4 m_projection;
  glsl::vec2 m_contrastGamma;
  glsl::vec2 m_position;
  float m_isOutlinePass = 1.0f;
  float m_opacity = 1.0f;
  float m_length = 0.0f;

  BIND_PROGRAMS(GuiProgramParams, Program::TextStaticOutlinedGui, Program::TextOutlinedGui, Program::TexturingGui,
                Program::Ruler)
};

struct ALIGNMENT ShapesProgramParams
{
  glsl::mat4 m_modelView;
  glsl::mat4 m_projection;
  glsl::mat4 m_pivotTransform;
  union
  {
    glsl::vec4 m_positionAligned;
    glsl::vec3 m_position;
  };
  glsl::vec2 m_lineParams;
  float m_accuracy = 0.0;
  float m_zScale = 1.0f;
  float m_opacity = 1.0f;
  float m_azimut = 0.0;

  ShapesProgramParams() : m_positionAligned(0.0f, 0.0f, 0.0f, 0.0f) {}

  BIND_PROGRAMS(ShapesProgramParams, Program::Accuracy, Program::MyPosition, Program::SelectionLine)
};

struct ALIGNMENT Arrow3dProgramParams
{
  glsl::mat4 m_transform;
  glsl::mat4 m_normalTransform;
  glsl::vec4 m_color;
  glsl::vec2 m_texCoordFlipping;

  BIND_PROGRAMS(Arrow3dProgramParams, Program::Arrow3d, Program::Arrow3dTextured, Program::Arrow3dShadow,
                Program::Arrow3dOutline)
};

struct ALIGNMENT DebugRectProgramParams
{
  glsl::vec4 m_color;

  BIND_PROGRAMS(DebugRectProgramParams, Program::DebugRect)
};

struct ALIGNMENT ScreenQuadProgramParams
{
  float m_opacity = 1.0f;
  float m_invertV = 1.0f;

  BIND_PROGRAMS(ScreenQuadProgramParams, Program::ScreenQuad)
};

struct ALIGNMENT SMAAProgramParams
{
  glsl::vec4 m_framebufferMetrics;

  BIND_PROGRAMS(SMAAProgramParams, Program::SmaaEdges, Program::SmaaBlendingWeight, Program::SmaaFinal)
};

struct ALIGNMENT ImGuiProgramParams
{
  glsl::mat4 m_projection;

  BIND_PROGRAMS(ImGuiProgramParams, Program::ImGui)
};

#undef ALIGNMENT

class ProgramParamsSetter
{
public:
  virtual ~ProgramParamsSetter() = default;
  virtual void Apply(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program,
                     MapProgramParams const & params) = 0;
  virtual void Apply(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program,
                     RouteProgramParams const & params) = 0;
  virtual void Apply(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program,
                     TrafficProgramParams const & params) = 0;
  virtual void Apply(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program,
                     TransitProgramParams const & params) = 0;
  virtual void Apply(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program,
                     GuiProgramParams const & params) = 0;
  virtual void Apply(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program,
                     ShapesProgramParams const & params) = 0;
  virtual void Apply(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program,
                     Arrow3dProgramParams const & params) = 0;
  virtual void Apply(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program,
                     DebugRectProgramParams const & params) = 0;
  virtual void Apply(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program,
                     ScreenQuadProgramParams const & params) = 0;
  virtual void Apply(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program,
                     SMAAProgramParams const & params) = 0;
  virtual void Apply(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::GpuProgram> program,
                     ImGuiProgramParams const & params) = 0;
};
}  // namespace gpu
