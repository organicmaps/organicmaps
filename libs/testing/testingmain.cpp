#include "testing/testing.hpp"
#include "testing/testregister.hpp"

#include "base/logging.hpp"
#include "base/timer.hpp"
#include "base/waiter.hpp"

#include "std/target_os.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#ifdef WITH_GL_MOCK
#include "base/scope_guard.hpp"
#include "drape/drape_tests/gl_mock_functions.hpp"
#endif

#ifdef OMIM_OS_IPHONE
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifndef OMIM_UNIT_TEST_DISABLE_PLATFORM_INIT
#include "platform/platform.hpp"
#endif

#if defined(OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP) && !defined(OMIM_OS_IPHONE)
#include <QtCore/Qt>
#ifdef OMIM_OS_MAC  // on Mac OS X native run loop works only for QApplication :(
#include <QtWidgets/QApplication>
#define QAPP QApplication
#else
#include <QtCore/QCoreApplication>
#define QAPP QCoreApplication
#endif
#endif

namespace testing
{
using namespace std;

base::Waiter g_waiter;

void RunEventLoop()
{
#if defined(OMIM_OS_IPHONE)
  CFRunLoopRun();
#elif defined(QAPP)
  QAPP::exec();
#endif
}

void StopEventLoop()
{
#if defined(OMIM_OS_IPHONE)
  CFRunLoopStop(CFRunLoopGetMain());
#elif defined(QAPP)
  QAPP::exit();
#endif
}

void Wait()
{
  g_waiter.Wait();
  g_waiter.Reset();
}

void Notify()
{
  g_waiter.Notify();
}

bool g_lastTestOK = true;
CommandLineOptions g_testingOptions;

int const kOptionFieldWidth = 32;
char const kFilterOption[] = "--filter=";
char const kSuppressOption[] = "--suppress=";
char const kHelpOption[] = "--help";
char const kDataPathOptions[] = "--data_path=";
char const kResourcePathOptions[] = "--user_resource_path=";
char const kListAllTestsOption[] = "--list_tests";

enum Status
{
  STATUS_SUCCESS = 0,
  STATUS_FAILED = 1,
  STATUS_BROKEN_FRAMEWORK = 5,
};

void DisplayOption(ostream & os, char const * option, char const * description)
{
  os << "  " << setw(kOptionFieldWidth) << left << option << " " << description << '\n';
}

void DisplayOption(ostream & os, char const * option, char const * value, char const * description)
{
  os << "  " << setw(kOptionFieldWidth) << left << (string(option) + value) << " " << description << '\n';
}

void Usage(char const * name)
{
  cerr << "USAGE: " << name << " [options]\n\n";
  cerr << "OPTIONS:\n";
  DisplayOption(cerr, kFilterOption, "<ECMA Regexp>", "Run tests with names corresponding to regexp.");
  DisplayOption(cerr, kSuppressOption, "<ECMA Regexp>", "Do not run tests with names corresponding to regexp.");
  DisplayOption(cerr, kDataPathOptions, "<Path>", "Path to data files.");
  DisplayOption(cerr, kResourcePathOptions, "<Path>", "Path to resources, styles and classificators.");
  DisplayOption(cerr, kListAllTestsOption, "List all the tests in the test suite and exit.");
  DisplayOption(cerr, kHelpOption, "Print this help message and exit.");
}

void ParseOptions(int argc, char * argv[], CommandLineOptions & options)
{
  for (int i = 1; i < argc; ++i)
  {
    std::string_view const arg = argv[i];
    if (arg.starts_with(kFilterOption))
      options.m_filterRegExp = argv[i] + sizeof(kFilterOption) - 1;
    if (arg.starts_with(kSuppressOption))
      options.m_suppressRegExp = argv[i] + sizeof(kSuppressOption) - 1;
    if (arg.starts_with(kDataPathOptions))
      options.m_dataPath = argv[i] + sizeof(kDataPathOptions) - 1;
    if (arg.starts_with(kResourcePathOptions))
      options.m_resourcePath = argv[i] + sizeof(kResourcePathOptions) - 1;
    if (arg == kHelpOption)
      options.m_help = true;
    if (arg == kListAllTestsOption)
      options.m_listTests = true;
  }
#ifndef OMIM_UNIT_TEST_DISABLE_PLATFORM_INIT
  // Setting stored paths from testingmain.cpp
  Platform & pl = GetPlatform();
  if (options.m_dataPath)
    pl.SetWritableDirForTests(options.m_dataPath);
  if (options.m_resourcePath)
  {
    pl.SetResourceDir(options.m_resourcePath);
    pl.SetSettingsDir(options.m_resourcePath);
  }
#endif
}

CommandLineOptions const & GetTestingOptions()
{
  return g_testingOptions;
}

int main(int argc, char * argv[])
{
#if defined(OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP) && !defined(OMIM_OS_IPHONE)
  QAPP theApp(argc, argv);
  UNUSED_VALUE(theApp);
#else
  UNUSED_VALUE(argc);
  UNUSED_VALUE(argv);
#endif

  base::ScopedLogLevelChanger const infoLogLevel(LINFO);
#if defined(OMIM_OS_DESKTOP) || defined(OMIM_OS_IPHONE)
  base::SetLogMessageFn(base::LogMessageTests);
#endif

#ifdef WITH_GL_MOCK
  emul::GLMockFunctions::Init(&argc, argv);
  SCOPE_GUARD(GLMockScope, bind(&emul::GLMockFunctions::Teardown));
#endif

  vector<string> testnames;
  vector<bool> testResults;
  int numFailedTests = 0;

  ParseOptions(argc, argv, g_testingOptions);
  if (g_testingOptions.m_help)
  {
    Usage(argv[0]);
    return STATUS_SUCCESS;
  }

  regex filterRegExp;
  if (g_testingOptions.m_filterRegExp)
    filterRegExp.assign(g_testingOptions.m_filterRegExp);

  regex suppressRegExp;
  if (g_testingOptions.m_suppressRegExp)
    suppressRegExp.assign(g_testingOptions.m_suppressRegExp);

  for (TestRegister * test = TestRegister::FirstRegister(); test; test = test->m_next)
  {
    string filename(test->m_filename);
    string testname(test->m_testname);

    // Retrieve fine file name.
    auto const lastSlash = filename.find_last_of("\\/");
    if (lastSlash != string::npos)
      filename.erase(0, lastSlash + 1);

    testnames.push_back(filename + "::" + testname);
    testResults.push_back(true);
  }

  if (GetTestingOptions().m_listTests)
  {
    for (auto const & name : testnames)
      cout << name << '\n';
    return 0;
  }

  int testIndex = 0;
  for (TestRegister * test = TestRegister::FirstRegister(); test; ++testIndex, test = test->m_next)
  {
    auto const & testname = testnames[testIndex];
    if (g_testingOptions.m_filterRegExp && !regex_search(testname.begin(), testname.end(), filterRegExp))
      continue;
    if (g_testingOptions.m_suppressRegExp && regex_search(testname.begin(), testname.end(), suppressRegExp))
      continue;

    LOG(LINFO, ("Running", testname));
    if (!g_lastTestOK)
    {
      // Somewhere else global variables have been reset.
      LOG(LERROR, ("\n\nSOMETHING IS REALLY WRONG IN THE UNIT TEST FRAMEWORK!!!"));
      return STATUS_BROKEN_FRAMEWORK;
    }

    base::HighResTimer timer(true);

    try
    {
      // Run the test.
      test->m_fn();
#ifdef WITH_GL_MOCK
      emul::GLMockFunctions::ValidateAndClear();
#endif

      if (g_lastTestOK)
      {
        LOG(LINFO, ("OK"));
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
    catch (exception const & ex)
    {
      LOG(LERROR, ("FAILED", "<<<Exception thrown [", ex.what(), "].>>>"));
      testResults[testIndex] = false;
      ++numFailedTests;
    }
    catch (...)
    {
      LOG(LERROR, ("FAILED<<<Unknown exception thrown.>>>"));
      testResults[testIndex] = false;
      ++numFailedTests;
    }
    g_lastTestOK = true;

    uint64_t const elapsed = timer.ElapsedNanoseconds();
    LOG(LINFO, ("Test took", elapsed / 1000000, "ms\n"));
  }

  if (numFailedTests != 0)
  {
    LOG(LINFO, (numFailedTests, " tests failed:"));
    for (size_t i = 0; i < testnames.size(); ++i)
      if (!testResults[i])
        LOG(LINFO, (testnames[i]));
    LOG(LINFO, ("Some tests FAILED."));
    return STATUS_FAILED;
  }

  LOG(LINFO, ("All tests passed."));
  return STATUS_SUCCESS;
}
}  // namespace testing

int main(int argc, char * argv[])
{
  return ::testing::main(argc, argv);
}
