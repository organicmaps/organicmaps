#include "qt_tstfrm/test_main_loop.hpp"
#include "testing/testing.hpp"

#include "shaders/gl_program_params.hpp"
#include "shaders/gl_program_pool.hpp"

#include "drape/drape_global.hpp"
#include "drape/drape_tests/testing_graphics_context.hpp"
#include "drape/gl_functions.hpp"

template <typename ProgramParams>
void TestProgramParams()
{
  auto constexpr api = dp::ApiVersion::OpenGLES3;
  TestingGraphicsContext context(api);
  GLFunctions::Init(api);
  gpu::GLProgramPool pool(api);
  gpu::GLProgramParamsSetter paramsSetter;

  ProgramParams params;
  for (auto const p : ProgramParams::GetBoundPrograms())
  {
    auto const program = pool.Get(p);
    program->Bind();
    paramsSetter.Apply(make_ref(&context), make_ref(program), params);
    program->Unbind();
  }
}

UNIT_TEST(MapProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("MapProgramParams_Test", &TestProgramParams<gpu::MapProgramParams>);
}

UNIT_TEST(RouteProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("RouteProgramParams_Test", &TestProgramParams<gpu::RouteProgramParams>);
}

UNIT_TEST(TrafficProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("TrafficProgramParams_Test", &TestProgramParams<gpu::TrafficProgramParams>);
}

UNIT_TEST(TransitProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("TransitProgramParams_Test", &TestProgramParams<gpu::TransitProgramParams>);
}

UNIT_TEST(GuiProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("GuiProgramParams_Test", &TestProgramParams<gpu::GuiProgramParams>);
}

UNIT_TEST(ShapesProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("ShapesProgramParams_Test", &TestProgramParams<gpu::ShapesProgramParams>);
}

UNIT_TEST(Arrow3dProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("Arrow3dProgramParams_Test", &TestProgramParams<gpu::Arrow3dProgramParams>);
}

UNIT_TEST(DebugRectProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("DebugRectProgramParams_Test", &TestProgramParams<gpu::DebugRectProgramParams>);
}

UNIT_TEST(ScreenQuadProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("ScreenQuadProgramParams_Test", &TestProgramParams<gpu::ScreenQuadProgramParams>);
}

UNIT_TEST(TileBackgroundProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("TileBackgroundProgramParams_Test",
                                      &TestProgramParams<gpu::TileBackgroundProgramParams>);
}

UNIT_TEST(SMAAProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("SMAAProgramParams_Test", &TestProgramParams<gpu::SMAAProgramParams>);
}
