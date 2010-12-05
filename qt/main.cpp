#include "mainwindow.hpp"

#include "../platform/platform.hpp"

#include "../base/logging.hpp"

//#include "../version/version.h"

#include <QtGui/QApplication>

#ifdef OMIM_OS_WINDOWS
  #include <../src/gui/image/qimageiohandler.h>
  #include <../src/network/bearer/qbearerplugin_p.h>
  #include <../src/corelib/plugin/qfactoryloader_p.h>
#endif

//#include <google/protobuf/stubs/common.h>

#include "../base/start_mem_debug.hpp"

#ifdef OMIM_OS_WINDOWS
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, imageLoader,
                          (QImageIOHandlerFactoryInterface_iid, QLatin1String("/imageformats")))
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, bearerLoader,
                          (QBearerEngineFactoryInterface_iid, QLatin1String("/bearer")))
#endif


namespace
{
  class InitializeFinalize
  {
  public:
    InitializeFinalize()
    {
      // App runs without error console under win32.
#if defined(OMIM_OS_WINDOWS) //&& defined(PROFILER_COMMON)
      freopen("mapswithme.log", "w", stderr);
      my::g_LogLevel = my::LDEBUG;

      //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF);
      //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    }
    ~InitializeFinalize()
    {
      // optional - clean allocated data in protobuf library
      // useful when using memory and resource leak utilites
      //google::protobuf::ShutdownProtobufLibrary();
    }
  };
}

int main(int argc, char *argv[])
{
  InitializeFinalize mainGuard;

  LOG(LINFO, ("MapsWithMe started"));
  //LOG(LINFO, ("Version : ", VERSION_STRING));
  //LOG(LINFO, ("Built on : ", VERSION_DATE_STRING));

  QApplication a(argc, argv);

  (void)GetPlatform();

  qt::MainWindow w;

  w.show();

  int const ret = a.exec();

  // QTBUG: Fix memory leaks. Nobody delete created plugins.
#ifdef OMIM_OS_WINDOWS
  QFactoryLoader * arr[] = { imageLoader(), bearerLoader() };
  for (int i = 0; i < 2; ++i)
  {
    QStringList const & keys = arr[i]->keys();

    for (int j = 0; j < keys.count(); ++j)
      delete arr[i]->instance(keys.at(j));
  }
#endif

  LOG(LINFO, ("MapsWithMe finished with code : ", ret));

  return ret;
}
