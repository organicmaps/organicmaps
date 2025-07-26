#include "qt_tstfrm/test_main_loop.hpp"
#include "testing/testing.hpp"

#include "shaders/gl_program_pool.hpp"

#include "drape/drape_global.hpp"
#include "drape/gl_functions.hpp"

#include <functional>

using namespace std::placeholders;

void CompileShaders(bool enableVTF)
{
  auto constexpr api = dp::ApiVersion::OpenGLES3;
  GLFunctions::Init(api);
  gpu::GLProgramPool pool(api);
  if (enableVTF)
    pool.SetDefines("#define ENABLE_VTF\n");

  for (size_t i = 0; i < static_cast<size_t>(gpu::Program::ProgramsCount); ++i)
    pool.Get(static_cast<gpu::Program>(i));
}

// These unit tests create Qt application and OGL context so can't be run in GUI-less Linux machine.
#ifdef OMIM_OS_MAC
UNIT_TEST(DesktopCompileShaders_GLES3_Test)
{
  RunTestInOpenGLOffscreenEnvironment("DesktopCompileShaders_GLES3_Test",
                                      std::bind(&CompileShaders, false /* enableVTF */));
}

UNIT_TEST(DesktopCompileShaders_GLES3_VTF_Test)
{
  RunTestInOpenGLOffscreenEnvironment("DesktopCompileShaders_GLES3_VTF_Test",
                                      std::bind(&CompileShaders, true /* enableVTF */));
}
#endif
