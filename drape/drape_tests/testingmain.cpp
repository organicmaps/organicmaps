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

static bool g_bLastTestOK = true;

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

  std::vector<std::string> testNames;
  std::vector<bool> testResults;
  int numFailedTests = 0;

  for (TestRegister * pTest = TestRegister::FirstRegister(); pTest; pTest = pTest->m_pNext)
  {
    std::string fileName(pTest->m_FileName);
    std::string testName(pTest->m_TestName);
    // Retrieve fine file name
    {
      int iFirstSlash = static_cast<int>(fileName.size()) - 1;
      while (iFirstSlash >= 0 && fileName[iFirstSlash] != '\\'  && fileName[iFirstSlash] != '/')
        --iFirstSlash;
      if (iFirstSlash >= 0)
        fileName.erase(0, iFirstSlash + 1);
    }

    testNames.push_back(fileName + "::" + testName);
    testResults.push_back(true);
  }

  int iTest = 0;
  for (TestRegister * pTest = TestRegister::FirstRegister(); pTest; ++iTest, pTest = pTest->m_pNext)
  {
    std::cerr << "Running " << testNames[iTest] << std::endl << std::flush;
    if (!g_bLastTestOK)
    {
      // Somewhere else global variables have been reset.
      std::cerr << "\n\nSOMETHING IS REALLY WRONG IN THE UNIT TEST FRAMEWORK!!!" << std::endl;
      return 5;
    }
    try
    {
      // Run the test.
      pTest->m_Fn();
      emul::GLMockFunctions::ValidateAndClear();

      if (g_bLastTestOK)
      {
        std::cerr << "OK" << std::endl;
      }
      else
      {
        // You can set Break here if test failed,
        // but it is already set in OnTestFail - to fail immediately.
        testResults[iTest] = false;
        ++numFailedTests;
      }

    }
    catch (TestFailureException const &)
    {
      testResults[iTest] = false;
      ++numFailedTests;
    }
    catch (std::exception const & ex)
    {
      std::cerr << "FAILED" << endl << "<<<Exception thrown [" << ex.what() << "].>>>" << std::endl;
      testResults[iTest] = false;
      ++numFailedTests;
    }
    catch (...)
    {
      std::cerr << "FAILED" << endl << "<<<Unknown exception thrown.>>>" << std::endl;
      testResults[iTest] = false;
      ++numFailedTests;
    }
    g_bLastTestOK = true;
  }

  if (numFailedTests == 0)
  {
    std::cerr << std::endl << "All tests passed." << std::endl << std::flush;
    return 0;
  }
  else
  {
    std::cerr << std::endl << numFailedTests << " tests failed:" << std::endl;
    for (size_t i = 0; i < testNames.size(); ++i)
    {
      if (!testResults[i])
        std::cerr << testNames[i] << std::endl;
    }
    std::cerr << "Some tests FAILED." << std::endl << flush;
    return 1;
  }
}
