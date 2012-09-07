#include "mainwindow.hpp"
#include "info_dialog.hpp"

#include "../platform/settings.hpp"
#include "../platform/platform.hpp"

#include "../base/logging.hpp"
#include "../base/macros.hpp"
#include "../base/object_tracker.hpp"

#include "../coding/file_reader.hpp"

#include "../version/version.hpp"

#include "../std/cstdio.hpp"

#include <QtGui/QApplication>
#include <QtCore/QLocale>

//#include <google/protobuf/stubs/common.h>

#define SETTING_EULA_ACCEPTED "EulaAccepted"

namespace
{
  class FinalizeBase
  {
  public:
    ~FinalizeBase()
    {
      // optional - clean allocated data in protobuf library
      // useful when using memory and resource leak utilites
      //google::protobuf::ShutdownProtobufLibrary();
    }
  };

#if defined(OMIM_OS_WINDOWS) //&& defined(PROFILER_COMMON)
  class InitializeFinalize : public FinalizeBase
  {
    FILE * m_errFile;
  public:
    InitializeFinalize()
    {
      // App runs without error console under win32.
      m_errFile = ::freopen(".\\mapswithme.log", "w", stderr);
      my::g_LogLevel = my::LDEBUG;

      //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF);
      //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    }
    ~InitializeFinalize()
    {
      ::fclose(m_errFile);
    }
  };
#else
  typedef FinalizeBase InitializeFinalize;
#endif
}

int main(int argc, char * argv[])
{
  InitializeFinalize mainGuard;
  UNUSED_VALUE(mainGuard);

  LOG_SHORT(LINFO, ("Started MapsWithMe version", VERSION_STRING));
  LOG_SHORT(LINFO, ("Built on", VERSION_DATE_STRING));

  QApplication a(argc, argv);

  (void)GetPlatform();

  // checking default measurement system.

  Settings::Units u;

  if (!Settings::Get("Units", u))
  {
    // set default measurement from system locale
    if (QLocale::system().measurementSystem() == QLocale::MetricSystem)
      u = Settings::Metric;
    else
      u = Settings::Foot;
  }

  Settings::Set("Units", u);

  // display EULA if needed
  bool eulaAccepted = false;
  if (!Settings::Get(SETTING_EULA_ACCEPTED, eulaAccepted) || !eulaAccepted)
  {
    QStringList buttons;
    buttons << "Accept" << "Decline";

    string buffer;
    {
      ReaderPtr<Reader> reader = GetPlatform().GetReader("eula.html");
      reader.ReadAsString(buffer);
    }
    qt::InfoDialog eulaDialog("MapsWithMe End User Licensing Agreement", buffer.c_str(), NULL, buttons);
    eulaAccepted = (eulaDialog.exec() == 1);
    Settings::Set(SETTING_EULA_ACCEPTED, eulaAccepted);
  }

  int returnCode = -1;
  if (eulaAccepted)   // User has accepted EULA
  {
    qt::MainWindow w;
    w.show();
    returnCode = a.exec();
  }

  dbg::ObjectTracker::PrintLeaks();

  LOG_SHORT(LINFO, ("MapsWithMe finished with code", returnCode));
  return returnCode;
}
