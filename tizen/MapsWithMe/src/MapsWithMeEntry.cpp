//
//This file contains the Tizen application entry point by default. You do not need to modify this file.
//
#include <new>
#include "MapsWithMeApp.h"

using namespace Tizen::Base;
using namespace Tizen::Base::Collection;

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

//
// The framework calls this method as the entry method of the Tizen application.
//
_EXPORT_ int OspMain(int argc, char* pArgv[])
{
  AppLog("Application started.");
  ArrayList args(SingleObjectDeleter);
  args.Construct();
  for (int i = 0; i < argc; i++)
  {
    args.Add(new String(pArgv[i]));
  }

  result r = Tizen::App::UiApp::Execute(MapsWithMeApp::CreateInstance, &args);
  TryLog(r == E_SUCCESS, "[%s] Application execution failed.", GetErrorMessage(r));
  AppLog("Application finished.");

  return static_cast< int >(r);
}
#ifdef __cplusplus
}
#endif // __cplusplus
