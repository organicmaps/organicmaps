#include "testing/testing.hpp"
#include "testing/testregister.hpp"

#include "base/logging.hpp"
#include "base/regexp.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "std/cstring.hpp"
#include "std/iomanip.hpp"
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

namespace
{
bool g_bLastTestOK = true;

int const kOptionFieldWidth = 32;
char const kFilterOption[] = "--filter=";
char const kSuppressOption[] = "--suppress=";
char const kHelpOption[] = "--help";

enum Status
{
  STATUS_SUCCESS = 0,
  STATUS_FAILED = 1,
  STATUS_BROKEN_FRAMEWORK = 5,
};

// This struct contains parsed command line options. It may contain pointers to argc contents.
struct CommandLineOptions
{
  CommandLineOptions() : filterRegExp(nullptr), suppressRegExp(nullptr), help(false) {}

  // Non-owning ptr.
  char const * filterRegExp;

  // Non-owning ptr.
  char const * suppressRegExp;

  bool help;
};

void DisplayOption(ostream & os, char const * option, char const * description)
{
  os << "  " << setw(kOptionFieldWidth) << left << option << " " << description << endl;
}

void DisplayOption(ostream & os, char const * option, char const * value, char const * description)
{
  os << "  " << setw(kOptionFieldWidth) << left << (string(option) + string(value)) << " "
     << description << endl;
}

void Usage(char const * name)
{
  cerr << "USAGE: " << name << " [options]" << endl;
  cerr << endl;
  cerr << "OPTIONS:" << endl;
  DisplayOption(cerr, kFilterOption, "<ECMA Regexp>",
                "Run tests with names corresponding to regexp.");
  DisplayOption(cerr, kSuppressOption, "<ECMA Regexp>",
                "Do not run tests with names corresponding to regexp.");
  DisplayOption(cerr, kHelpOption, "Print this help message and exit.");
}

void ParseOptions(int argc, char * argv[], CommandLineOptions & options)
{
  for (int i = 1; i < argc; ++i)
  {
    char const * const arg = argv[i];
    if (strings::StartsWith(arg, kFilterOption))
      options.filterRegExp = arg + sizeof(kFilterOption) - 1;
    if (strings::StartsWith(arg, kSuppressOption))
      options.suppressRegExp = arg + sizeof(kSuppressOption) - 1;
    if (strcmp(arg, kHelpOption) == 0)
      options.help = true;
  }
}
}  // namespace

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

  CommandLineOptions options;
  ParseOptions(argc, argv, options);
  if (options.help)
  {
    Usage(argv[0]);
    return STATUS_SUCCESS;
  }

  regexp::RegExpT filterRegExp;
  if (options.filterRegExp)
    regexp::Create(options.filterRegExp, filterRegExp);

  regexp::RegExpT suppressRegExp;
  if (options.suppressRegExp)
    regexp::Create(options.suppressRegExp, suppressRegExp);

  for (TestRegister * pTest = TestRegister::FirstRegister(); pTest; pTest = pTest->m_pNext)
  {
    string fileName(pTest->m_FileName);
    string testName(pTest->m_TestName);

    // Retrieve fine file name
    auto const lastSlash = fileName.find_last_of("\\/");
    if (lastSlash != string::npos)
      fileName.erase(0, lastSlash + 1);

    testNames.push_back(fileName + "::" + testName);
    testResults.push_back(true);
  }

  int iTest = 0;
  for (TestRegister * pTest = TestRegister::FirstRegister(); pTest; ++iTest, pTest = pTest->m_pNext)
  {
    if (options.filterRegExp && !regexp::Matches(testNames[iTest], filterRegExp))
      continue;
    if (options.suppressRegExp && regexp::Matches(testNames[iTest], suppressRegExp))
      continue;

    cerr << "Running " << testNames[iTest] << endl;
    if (!g_bLastTestOK)
    {
      // Somewhere else global variables have been reset.
      LOG(LERROR, ("\n\nSOMETHING IS REALLY WRONG IN THE UNIT TEST FRAMEWORK!!!"));
      return STATUS_BROKEN_FRAMEWORK;
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

  if (numFailedTests != 0)
  {
    LOG(LINFO, (numFailedTests, " tests failed:"));
    for (size_t i = 0; i < testNames.size(); ++i)
    {
      if (!testResults[i])
        cerr << testNames[i] << endl;
    }
    LOG(LINFO, ("Some tests FAILED."));
    return STATUS_FAILED;
  }

  LOG(LINFO, ("All tests passed."));
  return STATUS_SUCCESS;
}
