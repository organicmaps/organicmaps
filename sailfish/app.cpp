#include "sailfish/app_info.hpp"
#include "sailfish/app_settings.hpp"
#include "sailfish/countries_model.hpp"
#include "sailfish/framework_access.hpp"
#include "sailfish/map_item.hpp"
#include "sailfish/maps_storage.hpp"
#include "sailfish/place_page.hpp"
#include "sailfish/search_model.hpp"
#include "sailfish/url_handler.hpp"

#include "map/framework.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "base/logging.hpp"

#include <qqml.h>
#include <QDir>
#include <QEventLoop>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickView>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QTimer>

#include <atomic>
#include <clocale>
#include <cstdlib>
#include <memory>
#include <thread>

#include <sailfishapp.h>

namespace
{
Framework * g_framework = nullptr;

void SetEnvIfUnset(char const * name, QString const & value)
{
  if (qEnvironmentVariableIsEmpty(name))
    qputenv(name, value.toUtf8());
}

void SetDefaultSurfaceFormat()
{
  // Drape's contexts share resources with the scene graph through the global share context.
  QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QSurfaceFormat format;
  format.setRenderableType(QSurfaceFormat::OpenGLES);
  format.setVersion(3, 0);
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  QSurfaceFormat::setDefaultFormat(format);
}

// The copy can take minutes, e.g. to a slow memory card, so its progress is shown meanwhile. Without text: the
// strings come from the platform, which may only start once the maps folder is known.
QString MoveMaps()
{
  if (!sailfish::MapsStorage::HasPendingMove())
    return sailfish::MapsStorage::ApplyPendingMove([](double) {});

  std::unique_ptr<QQuickView> view(SailfishApp::createView());
  view->setSource(SailfishApp::pathTo(QStringLiteral("qml/MovingMaps.qml")));
  view->show();
  std::atomic<double> progress{0};
  QString dir;
  QEventLoop loop;
  std::thread worker([&]
  {
    dir = sailfish::MapsStorage::ApplyPendingMove([&](double copied) { progress = copied; });
    QMetaObject::invokeMethod(&loop, "quit", Qt::QueuedConnection);
  });
  QTimer timer;
  QObject::connect(&timer, &QTimer::timeout, [&] { view->rootObject()->setProperty("progress", progress.load()); });
  timer.start(500);
  loop.exec();
  worker.join();
  return dir;
}

void SetupPlatform()
{
  // Platform reads these on first use; the defaults match the installed RPM layout and Sailjail.
  SetEnvIfUnset("MWM_RESOURCES_DIR", SailfishApp::pathTo(QStringLiteral("data")).toLocalFile());
  QString writableDir = MoveMaps();
  if (writableDir.isEmpty())
    writableDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(writableDir);
  SetEnvIfUnset("MWM_WRITABLE_DIR", writableDir);

  sailfish::AppSettings::InitLogging();
  LOG(LINFO, ("Resources:", GetPlatform().ResourcesDir(), "Writable:", GetPlatform().WritableDir(),
              "Settings:", GetPlatform().SettingsDir()));
}

void RegisterQmlTypes()
{
  qmlRegisterType<sailfish::MapItem>("app.organicmaps", 1, 0, "MapItem");
  qmlRegisterType<sailfish::CountriesModel>("app.organicmaps", 1, 0, "CountriesModel");
  qmlRegisterType<sailfish::SearchModel>("app.organicmaps", 1, 0, "SearchModel");
  qmlRegisterUncreatableType<sailfish::PlacePage>("app.organicmaps", 1, 0, "PlacePage", "Owned by MapItem");
  qmlRegisterUncreatableType<sailfish::AppSettings>("app.organicmaps", 1, 0, "AppSettings", "Use appSettings");
}
}  // namespace

namespace sailfish
{
Framework & GetFramework()
{
  return *g_framework;
}
}  // namespace sailfish

__attribute__((visibility("default"))) int OrganicMapsMain(int argc, char * argv[])
{
  SetDefaultSurfaceFormat();

  std::unique_ptr<QGuiApplication> app(SailfishApp::application(argc, argv));
  // After the application, which sets the system locale: strtod() in platform/measurement_utils.cpp and
  // search/latlon_match.cpp needs dots as decimal separators.
  std::setlocale(LC_NUMERIC, "C");
  app->setOrganizationName(QStringLiteral("app.organicmaps"));
  app->setApplicationName(QStringLiteral("organicmaps"));

  QStringList const urls = app->arguments().mid(1);
  // Single instance: a second one would write the same settings, bookmarks and edits.
  sailfish::DBusInbox dbusInbox;
  if (!dbusInbox.Claim())
  {
    sailfish::DBusInbox::OpenInRunningApp(urls);
    return 0;
  }

  SetupPlatform();

  Framework framework;
  framework.SetupMeasurementSystem();
  framework.GetStorage().SetLocale(languages::GetCurrentTwine());
  g_framework = &framework;

  RegisterQmlTypes();

  // Declared before the view, which uses them until it is destroyed.
  sailfish::AppInfo appInfo;
  sailfish::AppSettings appSettings(framework);
  sailfish::MapsStorage mapsStorage(framework);
  sailfish::DownloadStatus downloads(framework.GetStorage());
  sailfish::UrlHandler urlHandler(framework);
  dbusInbox.SetHandler(urlHandler);
  std::unique_ptr<QQuickView> view(SailfishApp::createView());
  view->rootContext()->setContextProperty(QStringLiteral("appInfo"), &appInfo);
  view->rootContext()->setContextProperty(QStringLiteral("appSettings"), &appSettings);
  view->rootContext()->setContextProperty(QStringLiteral("urlHandler"), &urlHandler);
  view->rootContext()->setContextProperty(QStringLiteral("mapsStorage"), &mapsStorage);
  view->rootContext()->setContextProperty(QStringLiteral("downloads"), &downloads);
  // A local file loads synchronously.
  view->setSource(SailfishApp::pathToMainQml());
  if (view->status() != QQuickView::Ready)
  {
    for (auto const & error : view->errors())
      LOG(LERROR, (error.toString().toStdString()));
    return EXIT_FAILURE;
  }
  view->show();
  if (!urls.isEmpty())
    urlHandler.openUrl(urls);

  return app->exec();
}
