#include "testing/testing.hpp"
#include "testing/testregister.hpp"

#include "base/logging.hpp"
#include "base/regexp.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "std/iostream.hpp"
#include "std/target_os.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"


#ifdef OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP
  #include <Qt>
  #ifdef OMIM_OS_MAC // on Mac OS X native run loop works only for QApplication :(
    #if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
      #include <QtGui/QApplication>
    #else
      #include <QtWidgets/QApplication>
    #endif
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

  my::g_LogLevel = LINFO;
#if defined(OMIM_OS_MAC) || defined(OMIM_OS_LINUX)
  my::SetLogMessageFn(my::LogMessageTests);
#endif

  vector<string> testNames;
  vector<bool> testResults;
  int numFailedTests = 0;

  char const filterOptionPrefix[] = "--filter=";
  char const * testsFilter = nullptr;

  regexp::RegExpT testsFilterRegExp;

  for (int arg = 1; arg < argc; ++arg)
  {
    if (strings::StartsWith(argv[arg], filterOptionPrefix))
      testsFilter = argv[arg] + sizeof(filterOptionPrefix) - 1;
  }

  if (testsFilter)
    regexp::Create(testsFilter, testsFilterRegExp);

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
    if (testsFilter && !regexp::Matches(testNames[iTest], testsFilterRegExp))
      continue;
    cerr << "Running " << testNames[iTest] << endl << flush;
    if (!g_bLastTestOK)
    {
      // Somewhere else global variables have been reset.
      LOG(LERROR, ("\n\nSOMETHING IS REALLY WRONG IN THE UNIT TEST FRAMEWORK!!!"));
      return 5;
    }

    my::HighResTimer timer(true);

    try
    {
      // Run the test.
      pTest->m_Fn();

      if (g_bLastTestOK)
      {
        LOG(LINFO, ("OK"));
      }
      else
      {
        // You can set Break here if test failed,
        // but it is already set in OnTestFail - to fail immediately.
        testResults[iTest] = false;
        ++numFailedTests;
      }

    }
    catch (TestFailureException const & )
    {
      testResults[iTest] = false;
      ++numFailedTests;
    }
    catch (std::exception const & ex)
    {
      LOG(LERROR, ("FAILED", "<<<Exception thrown [", ex.what(), "].>>>"));
      testResults[iTest] = false;
      ++numFailedTests;
    }
    catch (...)
    {
      LOG(LERROR, ("FAILED<<<Unknown exception thrown.>>>"));
      testResults[iTest] = false;
      ++numFailedTests;
    }
    g_bLastTestOK = true;

    uint64_t const elapsed = timer.ElapsedNano();
    LOG(LINFO, ("Test took", elapsed / 1000000, "ms\n"));
  }

  if (numFailedTests == 0)
  {
    LOG(LINFO, ("All tests passed."));
    return 0;
  }
  else
  {
    LOG(LINFO, (numFailedTests, " tests failed:"));
    for (size_t i = 0; i < testNames.size(); ++i)
    {
      if (!testResults[i])
        cerr << testNames[i] << endl;
    }
    LOG(LINFO, ("Some tests FAILED."));
    return 1;
  }
}
