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

static bool g_bLastTestOK = true;

int run_all_tests()
{
  AppLog("Running all tests");

  vector<string> testNames;
  vector<bool> testResults;
  int numFailedTests = 0;

  for (TestRegister * pTest = TestRegister::FirstRegister(); pTest; pTest = pTest->m_pNext)
  {
    string fileName(pTest->m_FileName);
    string testName(pTest->m_TestName);
    {
      int iFirstSlash = static_cast<int>(fileName.size()) - 1;
      while (iFirstSlash >= 0 && fileName[iFirstSlash] != '\\' && fileName[iFirstSlash] != '/')
        --iFirstSlash;
      if (iFirstSlash >= 0)
        fileName.erase(0, iFirstSlash + 1);
    }

    testNames.push_back(fileName + "::" + testName);
    testResults.push_back(true);
  }

  int iTest = 0;
  int nPassedTests = 0;
  for (TestRegister * pTest = TestRegister::FirstRegister(); pTest; ++iTest, pTest = pTest->m_pNext)
  {
    string s = testNames[iTest];
    AppLog("////////////////////////////////////////////////////////////////////////");
    s = "Running test " + s;
    AppLog(s.c_str());
    AppLog("////////////////////////////////////////////////////////////////////////");

    if (!g_bLastTestOK)
    {
      AppLog("g_bLastTestOK - false");
      return 5;
    }
    try
    {
      // Run the test.
      pTest->m_Fn();
      AppLog("Passed");

      nPassedTests++;
      if (g_bLastTestOK)
      {
      }
      else
      {
        // You can set Break here if test failed,
        // but it is already set in OnTestFail - to fail immediately.
        testResults[iTest] = false;
        ++numFailedTests;
        std::ostringstream os;
        os << "Failed test " << testNames[iTest];
        AppLogException(os.str().c_str());
      }

    }

    catch (std::exception const & ex)
    {
      testResults[iTest] = false;
      ++numFailedTests;
      std::ostringstream os;
      os << "Failed test with std exception "<< ex.what() << " in test " << testNames[iTest];
      AppLogException(os.str().c_str());

    }
    catch (...)
    {
      testResults[iTest] = false;
      ++numFailedTests;
      std::ostringstream os;
      os << "Failed test with exception " << testNames[iTest];
      AppLogException(os.str().c_str());
    }
    g_bLastTestOK = true;
  }

  if (numFailedTests == 0)
  {
    return nPassedTests;
    return 0;
  }
  else
  {
    for (size_t i = 0; i < testNames.size(); ++i)
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
