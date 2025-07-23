#include "qt_tstfrm/test_main_loop.hpp"
#include "testing/testing.hpp"

#include "shaders/gl_program_params.hpp"
#include "shaders/gl_program_pool.hpp"

#include "drape/drape_global.hpp"
#include "drape/drape_tests/testing_graphics_context.hpp"
#include "drape/gl_functions.hpp"

#include "std/target_os.hpp"

#include <functional>

using namespace std::placeholders;

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

// These unit tests create Qt application and OGL context so can't be run in GUI-less Linux machine.
#ifdef OMIM_OS_MAC
UNIT_TEST(MapProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("MapProgramParams_Test", std::bind(&TestProgramParams<gpu::MapProgramParams>));
}

UNIT_TEST(RouteProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("RouteProgramParams_Test",
                                      std::bind(&TestProgramParams<gpu::RouteProgramParams>));
}

UNIT_TEST(TrafficProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("TrafficProgramParams_Test",
                                      std::bind(&TestProgramParams<gpu::TrafficProgramParams>));
}

UNIT_TEST(TransitProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("TransitProgramParams_Test",
                                      std::bind(&TestProgramParams<gpu::TransitProgramParams>));
}

UNIT_TEST(GuiProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("GuiProgramParams_Test", std::bind(&TestProgramParams<gpu::GuiProgramParams>));
}

UNIT_TEST(ShapesProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("ShapesProgramParams_Test",
                                      std::bind(&TestProgramParams<gpu::ShapesProgramParams>));
}

UNIT_TEST(Arrow3dProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("Arrow3dProgramParams_Test",
                                      std::bind(&TestProgramParams<gpu::Arrow3dProgramParams>));
}

UNIT_TEST(DebugRectProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("DebugRectProgramParams_Test",
                                      std::bind(&TestProgramParams<gpu::DebugRectProgramParams>));
}

UNIT_TEST(ScreenQuadProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("ScreenQuadProgramParams_Test",
                                      std::bind(&TestProgramParams<gpu::ScreenQuadProgramParams>));
}

UNIT_TEST(SMAAProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("SMAAProgramParams_Test", std::bind(&TestProgramParams<gpu::SMAAProgramParams>));
}
#endif
