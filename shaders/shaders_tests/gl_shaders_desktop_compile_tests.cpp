#include "qt_tstfrm/test_main_loop.hpp"
#include "testing/testing.hpp"

#include "shaders/gl_program_pool.hpp"

#include "drape/drape_global.hpp"
#include "drape/glfunctions.hpp"

#include <functional>

using namespace std::placeholders;

void CompileShaders(bool apiOpenGLES3, bool enableVTF)
{
  auto const api = apiOpenGLES3 ? dp::ApiVersion::OpenGLES3 : dp::ApiVersion::OpenGLES2;
  GLFunctions::Init(api);
  gpu::GLProgramPool pool(api);
  if (enableVTF)
    pool.SetDefines("#define ENABLE_VTF\n");

  for (size_t i = 0; i < static_cast<size_t>(gpu::Program::ProgramsCount); ++i)
    pool.Get(static_cast<gpu::Program>(i));
}

// These unit tests create Qt application and OGL context so can't be run in GUI-less Linux machine.
#ifdef OMIM_OS_MAC
UNIT_TEST(DesktopCompileShaders_GLES2_Test)
{
  RunTestInOpenGLOffscreenEnvironment("DesktopCompileShaders_GLES2_Test", false /* apiOpenGLES3 */,
                                      std::bind(&CompileShaders, _1, false /* enableVTF */));
}

UNIT_TEST(DesktopCompileShaders_GLES3_Test)
{
  RunTestInOpenGLOffscreenEnvironment("DesktopCompileShaders_GLES3_Test", true /* apiOpenGLES3 */,
                                      std::bind(&CompileShaders, _1, false /* enableVTF */));

}

UNIT_TEST(DesktopCompileShaders_GLES2_VTF_Test)
{
  RunTestInOpenGLOffscreenEnvironment("DesktopCompileShaders_GLES2_VTF_Test", false /* apiOpenGLES3 */,
                                      std::bind(&CompileShaders, _1, true /* enableVTF */));
}

UNIT_TEST(DesktopCompileShaders_GLES3_VTF_Test)
{
  RunTestInOpenGLOffscreenEnvironment("DesktopCompileShaders_GLES3_VTF_Test", true /* apiOpenGLES3 */,
                                      std::bind(&CompileShaders, _1, true /* enableVTF */));

}
#endif
