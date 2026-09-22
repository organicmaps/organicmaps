#include "shaders/program_params.hpp"

namespace gpu
{
std::map<std::string_view, std::string_view> const & ProgramParams::GetBindings()
{
  // Immutable metadata is shared by all renderers; GPU program lifetimes are independent.
  static auto const bindings = []
  {
    std::map<std::string_view, std::string_view> bindings;
    MapProgramParams::BindPrograms(bindings);
    RouteProgramParams::BindPrograms(bindings);
    TrafficProgramParams::BindPrograms(bindings);
    TransitProgramParams::BindPrograms(bindings);
    GuiProgramParams::BindPrograms(bindings);
    ShapesProgramParams::BindPrograms(bindings);
    Arrow3dProgramParams::BindPrograms(bindings);
    DebugRectProgramParams::BindPrograms(bindings);
    ScreenQuadProgramParams::BindPrograms(bindings);
    SMAAProgramParams::BindPrograms(bindings);
    TileBackgroundProgramParams::BindPrograms(bindings);
    ImGuiProgramParams::BindPrograms(bindings);
    return bindings;
  }();
  return bindings;
}

// static
std::string_view ProgramParams::GetBoundParamsName(ref_ptr<dp::GpuProgram> program)
{
  auto const & bindings = GetBindings();
  auto const it = bindings.find(program->GetName());
  ASSERT(it != bindings.cend(), (program->GetName(), "Program is not bound to params"));
  if (it == bindings.cend())
    return {};
  return it->second;
}
}  // namespace gpu
