#include "drape/drape_tests/gl_mock_functions.hpp"

namespace emul
{
void GLMockFunctions::Init(int * argc, char ** argv)
{
  ::testing::InitGoogleMock(argc, argv);
  m_mock = new GLMockFunctions();
}

void GLMockFunctions::Teardown()
{
  delete m_mock;
  m_mock = NULL;
}

GLMockFunctions & GLMockFunctions::Instance()
{
  return *m_mock;
}

void GLMockFunctions::ValidateAndClear()
{
  ::testing::Mock::VerifyAndClear(m_mock);
}

GLMockFunctions * GLMockFunctions::m_mock;
}  // namespace emul
