#include "testing/testregister.hpp"
#include "testing/testing.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "std/target_os.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "drape/drape_tests/gl_mock_functions.hpp"

#ifdef OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP
  #include <Qt>
  #ifdef OMIM_OS_MAC // on Mac OS X native run loop works only for QApplication :(
    #include <QtWidgets/QApplication>
    #define QAPP QApplication
  #else
    #include <QtCore/QCoreApplication>
    #define QAPP QCoreApplication
  #endif
#endif

static bool g_lastTestOK = true;

int main(int argc, char * argv[])
{
#ifdef OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP
  QAPP theApp(argc, argv);
  UNUSED_VALUE(theApp);
#else
  UNUSED_VALUE(argc);
  UNUSED_VALUE(argv);
#endif

  base::ScopedLogLevelChanger const infoLogLevel(LINFO);

  emul::GLMockFunctions::Init(&argc, argv);

  SCOPE_GUARD(GLMockScope, std::bind(&emul::GLMockFunctions::Teardown));

  std::vector<std::string> testnames;
  std::vector<bool> testResults;
  int numFailedTests = 0;

  for (TestRegister * test = TestRegister::FirstRegister(); test; test = test->m_next)
  {
    std::string filename(test->m_filename);
    std::string testname(test->m_testname);

    // Retrieve fine file name.
    auto const lastSlash = filename.find_last_of("\\/");
    if (lastSlash != std::string::npos)
      filename.erase(0, lastSlash + 1);

    testnames.push_back(filename + "::" + testname);
    testResults.push_back(true);
  }

  int testIndex = 0;
  for (TestRegister *test = TestRegister::FirstRegister(); test; ++testIndex, test = test->m_next)
  {
    std::cerr << "Running " << testnames[testIndex] << std::endl << std::flush;
    if (!g_lastTestOK)
    {
      // Somewhere else global variables have been reset.
      std::cerr << "\n\nSOMETHING IS REALLY WRONG IN THE UNIT TEST FRAMEWORK!!!" << std::endl;
      return 5;
    }
    try
    {
      // Run the test.
      test->m_fn();
      emul::GLMockFunctions::ValidateAndClear();

      if (g_lastTestOK)
      {
        std::cerr << "OK" << std::endl;
      }
      else
      {
        testResults[testIndex] = false;
        ++numFailedTests;
      }
    }
    catch (TestFailureException const &)
    {
      testResults[testIndex] = false;
      ++numFailedTests;
    }
    catch (std::exception const & ex)
    {
      std::cerr << "FAILED" << std::endl << "<<<Exception thrown [" << ex.what() << "].>>>" << std::endl;
      testResults[testIndex] = false;
      ++numFailedTests;
    }
    catch (...)
    {
      std::cerr << "FAILED" << std::endl << "<<<Unknown exception thrown.>>>" << std::endl;
      testResults[testIndex] = false;
      ++numFailedTests;
    }
    g_lastTestOK = true;
  }

  if (numFailedTests == 0)
  {
    std::cerr << std::endl << "All tests passed." << std::endl << std::flush;
    return 0;
  }
  else
  {
    std::cerr << std::endl << numFailedTests << " tests failed:" << std::endl;
    for (size_t i = 0; i < testnames.size(); ++i)
    {
      if (!testResults[i])
        std::cerr << testnames[i] << std::endl;
    }
    std::cerr << "Some tests FAILED." << std::endl << std::flush;
    return 1;
  }
}
