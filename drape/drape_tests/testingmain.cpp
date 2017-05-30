#include "testing/testregister.hpp"
#include "testing/testing.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "std/algorithm.hpp"
#include "std/iostream.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"
#include "std/target_os.hpp"
#include "std/bind.hpp"

#include "drape/drape_tests/glmock_functions.hpp"


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

  my::ScopedLogLevelChanger const infoLogLevel(LINFO);

  emul::GLMockFunctions::Init(&argc, argv);
  MY_SCOPE_GUARD(GLMockScope, bind(&emul::GLMockFunctions::Teardown));

  vector<string> testNames;
  vector<bool> testResults;
  int numFailedTests = 0;

  for (TestRegister * pTest = TestRegister::FirstRegister(); pTest; pTest = pTest->m_pNext)
  {
    string fileName(pTest->m_FileName);
    string testName(pTest->m_TestName);
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
    cerr << "Running " << testNames[iTest] << endl << flush;
    if (!g_bLastTestOK)
    {
      // Somewhere else global variables have been reset.
      cerr << "\n\nSOMETHING IS REALLY WRONG IN THE UNIT TEST FRAMEWORK!!!" << endl;
      return 5;
    }
    try
    {
      // Run the test.
      pTest->m_Fn();
      emul::GLMockFunctions::ValidateAndClear();

      if (g_bLastTestOK)
      {
        cerr << "OK" << endl;
      }
      else
      {
        // You can set Break here if test failed,
        // but it is already set in OnTestFail - to fail immediately.
        testResults[iTest] = false;
        ++numFailedTests;
      }

    } catch (TestFailureException const & )
    {
      testResults[iTest] = false;
      ++numFailedTests;
    } catch (std::exception const & ex)
    {
      cerr << "FAILED" << endl << "<<<Exception thrown [" << ex.what() << "].>>>" << endl;
      testResults[iTest] = false;
      ++numFailedTests;
    } catch (...)
    {
      cerr << "FAILED" << endl << "<<<Unknown exception thrown.>>>" << endl;
      testResults[iTest] = false;
      ++numFailedTests;
    }
    g_bLastTestOK = true;
  }

  if (numFailedTests == 0)
  {
    cerr << endl << "All tests passed." << endl << flush;
    return 0;
  }
  else
  {
    cerr << endl << numFailedTests << " tests failed:" << endl;
    for (size_t i = 0; i < testNames.size(); ++i)
    {
      if (!testResults[i])
        cerr << testNames[i] << endl;
    }
    cerr << "Some tests FAILED." << endl << flush;
    return 1;
  }
}
