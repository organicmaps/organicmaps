#include "qt_tstfrm/test_main_loop.hpp"
#include "testing/testing.hpp"

#include "shaders/gl_program_params.hpp"
#include "shaders/gl_program_pool.hpp"

#include "drape/drape_global.hpp"
#include "drape/drape_tests/testing_graphics_context.hpp"
#include "drape/gl_functions.hpp"

#include <functional>

using namespace std::placeholders;

template <typename ProgramParams>
void TestProgramParams(bool apiOpenGLES3)
{
  auto const api = apiOpenGLES3 ? dp::ApiVersion::OpenGLES3 : dp::ApiVersion::OpenGLES2;
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
  RunTestInOpenGLOffscreenEnvironment("MapProgramParams_Test", false /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::MapProgramParams>, _1));

  RunTestInOpenGLOffscreenEnvironment("MapProgramParams_Test", true /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::MapProgramParams>, _1));
}

UNIT_TEST(RouteProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("RouteProgramParams_Test", false /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::RouteProgramParams>, _1));

  RunTestInOpenGLOffscreenEnvironment("RouteProgramParams_Test", true /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::RouteProgramParams>, _1));
}

UNIT_TEST(TrafficProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("TrafficProgramParams_Test", false /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::TrafficProgramParams>, _1));

  RunTestInOpenGLOffscreenEnvironment("TrafficProgramParams_Test", true /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::TrafficProgramParams>, _1));
}

UNIT_TEST(TransitProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("TransitProgramParams_Test", false /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::TransitProgramParams>, _1));

  RunTestInOpenGLOffscreenEnvironment("TransitProgramParams_Test", true /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::TransitProgramParams>, _1));
}

UNIT_TEST(GuiProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("GuiProgramParams_Test", false /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::GuiProgramParams>, _1));

  RunTestInOpenGLOffscreenEnvironment("GuiProgramParams_Test", true /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::GuiProgramParams>, _1));
}

UNIT_TEST(ShapesProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("ShapesProgramParams_Test", false /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::ShapesProgramParams>, _1));

  RunTestInOpenGLOffscreenEnvironment("ShapesProgramParams_Test", true /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::ShapesProgramParams>, _1));
}

UNIT_TEST(Arrow3dProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("Arrow3dProgramParams_Test", false /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::Arrow3dProgramParams>, _1));

  RunTestInOpenGLOffscreenEnvironment("Arrow3dProgramParams_Test", true /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::Arrow3dProgramParams>, _1));
}

UNIT_TEST(DebugRectProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("DebugRectProgramParams_Test", false /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::DebugRectProgramParams>, _1));

  RunTestInOpenGLOffscreenEnvironment("DebugRectProgramParams_Test", true /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::DebugRectProgramParams>, _1));
}

UNIT_TEST(ScreenQuadProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("ScreenQuadProgramParams_Test", false /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::ScreenQuadProgramParams>, _1));

  RunTestInOpenGLOffscreenEnvironment("ScreenQuadProgramParams_Test", true /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::ScreenQuadProgramParams>, _1));
}

UNIT_TEST(SMAAProgramParams_Test)
{
  RunTestInOpenGLOffscreenEnvironment("SMAAProgramParams_Test", false /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::SMAAProgramParams>, _1));

  RunTestInOpenGLOffscreenEnvironment("SMAAProgramParams_Test", true /* apiOpenGLES3 */,
                                      std::bind(&TestProgramParams<gpu::SMAAProgramParams>, _1));
}
#endif
