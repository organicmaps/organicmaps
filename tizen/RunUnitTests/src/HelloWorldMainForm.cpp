#include "HelloWorldMainForm.h"
#include "AppResourceId.h"
#include <FBase.h>
#include "/Users/Sergey/GitHub/omim/testing/testregister.hpp"
#include "/Users/Sergey/GitHub/omim/platform/platform.hpp"

#include <vector>

using namespace Tizen::Base;
using namespace Tizen::App;
using namespace Tizen::Ui;
using namespace Tizen::Ui::Controls;
using namespace Tizen::Ui::Scenes;

using namespace std;

static bool g_lastTestOK = true;

int run_all_tests()
{
  AppLog("Running all tests");

  vector<string> testnames;
  vector<bool> testResults;
  int numFailedTests = 0;

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

  int testIndex = 0;
  int nPassedTests = 0;
  for (TestRegister *test = TestRegister::FirstRegister(); test; ++testIndex, test = test->m_next)
  {
    string s = testnames[testIndex];
    AppLog("////////////////////////////////////////////////////////////////////////");
    s = "Running test " + s;
    AppLog(s.c_str());
    AppLog("////////////////////////////////////////////////////////////////////////");

    if (!g_lastTestOK)
    {
      AppLog("g_lastTestOK - false");
      return 5;
    }
    try
    {
      // Run the test.
      test->m_fn();
      AppLog("Passed");

      nPassedTests++;
      if (g_lastTestOK)
      {
      }
      else
      {
        testResults[testIndex] = false;
        ++numFailedTests;
        std::ostringstream os;
        os << "Failed test " << testnames[testIndex];
        AppLogException(os.str().c_str());
      }
    }

    catch (std::exception const & ex)
    {
      testResults[testIndex] = false;
      ++numFailedTests;
      std::ostringstream os;
      os << "Failed test with std exception " << ex.what() << " in test " << testnames[testIndex];
      AppLogException(os.str().c_str());

    }
    catch (...)
    {
      testResults[testIndex] = false;
      ++numFailedTests;
      std::ostringstream os;
      os << "Failed test with exception " << testnames[testIndex];
      AppLogException(os.str().c_str());
    }
    g_lastTestOK = true;
  }

  if (numFailedTests == 0)
  {
    return nPassedTests;
  }
  else
  {
    for (size_t i = 0; i < testnames.size(); ++i)
    {
    }
    return numFailedTests * 10000 + nPassedTests;
  }
}

HelloWorldMainForm::HelloWorldMainForm(void)
{
}

HelloWorldMainForm::~HelloWorldMainForm(void)
{
}

bool HelloWorldMainForm::Initialize(void)
{
  result r = Construct(IDL_FORM);
  TryReturn(r == E_SUCCESS, false, "Failed to construct form");

  return true;
}

result HelloWorldMainForm::OnInitializing(void)
{
  result r = E_SUCCESS;

  // TODO: Add your initialization code here

  // Setup back event listener
  SetFormBackEventListener(this);

  // Get a button via resource ID
  Tizen::Ui::Controls::Button* pButtonOk = static_cast<Button*>(GetControl(IDC_BUTTON_OK));
  if (pButtonOk != null)
  {
    pButtonOk->SetActionId(IDA_BUTTON_OK);
    pButtonOk->AddActionEventListener(*this);
  }

  return r;
}

result HelloWorldMainForm::OnTerminating(void)
{
  result r = E_SUCCESS;

  // TODO: Add your termination code here
  return r;
}

void HelloWorldMainForm::OnActionPerformed(const Tizen::Ui::Control& source, int actionId)
{
  SceneManager* pSceneManager = SceneManager::GetInstance();
  AppAssert(pSceneManager);

  switch (actionId)
  {
    case IDA_BUTTON_OK:
    {
      int m = run_all_tests();
      if (m != 0)
      {
        std::ostringstream os;
        os << m << "Tests passed.";
        AppLog(os.str().c_str());
      }
      else
        AppLog("Tests failed");
    }
      break;

    default:
      break;
  }
}

void HelloWorldMainForm::OnFormBackRequested(Tizen::Ui::Controls::Form& source)
{
  UiApp* pApp = UiApp::GetInstance();
  AppAssert(pApp);
  pApp->Terminate();
}

void HelloWorldMainForm::OnSceneActivatedN(const Tizen::Ui::Scenes::SceneId& previousSceneId,
    const Tizen::Ui::Scenes::SceneId& currentSceneId, Tizen::Base::Collection::IList* pArgs)
{
  // TODO: Activate your scene here.

}

void HelloWorldMainForm::OnSceneDeactivated(const Tizen::Ui::Scenes::SceneId& currentSceneId,
    const Tizen::Ui::Scenes::SceneId& nextSceneId)
{
  // TODO: Deactivate your scene here.

}
