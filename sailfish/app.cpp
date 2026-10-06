#include "sailfish/app_info.hpp"
#include "sailfish/app_settings.hpp"
#include "sailfish/bookmark_editor.hpp"
#include "sailfish/bookmarks_io.hpp"
#include "sailfish/bookmarks_model.hpp"
#include "sailfish/countries_model.hpp"
#include "sailfish/framework_access.hpp"
#include "sailfish/map_item.hpp"
#include "sailfish/maps_storage.hpp"
#include "sailfish/opening_hours_editor.hpp"
#include "sailfish/osm_account.hpp"
#include "sailfish/place_editor.hpp"
#include "sailfish/place_page.hpp"
#include "sailfish/routing.hpp"
#include "sailfish/search_model.hpp"
#include "sailfish/url_handler.hpp"

#include "map/framework.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "base/logging.hpp"

#include <qqml.h>
#include <QDir>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QStandardPaths>
#include <QSurfaceFormat>

#include <clocale>
#include <memory>

#include <sailfishapp.h>

namespace
{
Framework * g_framework = nullptr;

void SetEnvIfUnset(char const * name, QString const & value)
{
  if (qEnvironmentVariableIsEmpty(name))
    qputenv(name, value.toUtf8());
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
  // Our double parsing code (base/string_utils.hpp) needs dots as a floating point delimiters.
  std::setlocale(LC_NUMERIC, "C");

  // Drape's contexts share resources with the scene graph through the global share context.
  QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QSurfaceFormat format;
  format.setRenderableType(QSurfaceFormat::OpenGLES);
  format.setVersion(3, 0);
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  QSurfaceFormat::setDefaultFormat(format);

  std::unique_ptr<QGuiApplication> app(SailfishApp::application(argc, argv));
  app->setOrganizationName(QStringLiteral("app.organicmaps"));
  app->setApplicationName(QStringLiteral("organicmaps"));

  QStringList const urls = app->arguments().mid(1);
  // Single instance: a second one would write the same settings, bookmarks and edits.
  if (!sailfish::UrlHandler::ClaimService())
  {
    sailfish::UrlHandler::OpenInRunningApp(urls);
    return 0;
  }

  // Platform reads these on first use; the defaults match the installed RPM layout and Sailjail.
  SetEnvIfUnset("MWM_RESOURCES_DIR", SailfishApp::pathTo(QStringLiteral("data")).toLocalFile());
  QString writableDir = sailfish::MapsStorage::ConfiguredDir();
  if (writableDir.isEmpty())
    writableDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(writableDir);
  SetEnvIfUnset("MWM_WRITABLE_DIR", writableDir);

  sailfish::AppSettings::InitLogging();
  LOG(LINFO, ("Resources:", GetPlatform().ResourcesDir(), "Writable:", GetPlatform().WritableDir(),
              "Settings:", GetPlatform().SettingsDir()));

  Framework framework;
  framework.SetupMeasurementSystem();
  framework.GetStorage().SetLocale(languages::GetCurrentTwine());
  g_framework = &framework;
  sailfish::BookmarksNotifier::LoadBookmarks(framework);

  qmlRegisterType<sailfish::MapItem>("app.organicmaps", 1, 0, "MapItem");
  qmlRegisterType<sailfish::CountriesModel>("app.organicmaps", 1, 0, "CountriesModel");
  qmlRegisterType<sailfish::SearchModel>("app.organicmaps", 1, 0, "SearchModel");
  qmlRegisterType<sailfish::BookmarkCategoriesModel>("app.organicmaps", 1, 0, "BookmarkCategoriesModel");
  qmlRegisterType<sailfish::BookmarksModel>("app.organicmaps", 1, 0, "BookmarksModel");
  qmlRegisterType<sailfish::BookmarkEditor>("app.organicmaps", 1, 0, "BookmarkEditor");
  qmlRegisterType<sailfish::PlaceEditor>("app.organicmaps", 1, 0, "PlaceEditor");
  qmlRegisterType<sailfish::PlaceCategories>("app.organicmaps", 1, 0, "PlaceCategories");
  qmlRegisterType<sailfish::OpeningHoursEditor>("app.organicmaps", 1, 0, "OpeningHoursEditor");
  qmlRegisterUncreatableType<sailfish::PlacePage>("app.organicmaps", 1, 0, "PlacePage", "Owned by MapItem");
  qmlRegisterUncreatableType<sailfish::Routing>("app.organicmaps", 1, 0, "Routing", "Owned by MapItem");
  qmlRegisterUncreatableType<sailfish::AppSettings>("app.organicmaps", 1, 0, "AppSettings", "Use appSettings");
  qmlRegisterUncreatableType<sailfish::BookmarksIO>("app.organicmaps", 1, 0, "BookmarksIO", "Use bookmarksIO");

  // Declared before the view, which uses them until it is destroyed.
  sailfish::AppInfo appInfo;
  sailfish::AppSettings appSettings(framework);
  sailfish::OsmAccount osmAccount;
  sailfish::BookmarksIO bookmarksIO(framework);
  sailfish::MapsStorage mapsStorage(framework);
  sailfish::DownloadStatus downloads(framework.GetStorage());
  sailfish::UrlHandler urlHandler(framework, bookmarksIO, mapsStorage);
  // The OpenStreetMap login in the browser returns by an om:// link.
  QObject::connect(&urlHandler, &sailfish::UrlHandler::oauth2CodeReceived, &osmAccount,
                   &sailfish::OsmAccount::LoginWithCode);
  if (!urlHandler.RegisterOnDBus())
    LOG(LWARNING, ("Can't serve the D-Bus interface: links from other apps won't open"));
  std::unique_ptr<QQuickView> view(SailfishApp::createView());
  view->rootContext()->setContextProperty(QStringLiteral("appInfo"), &appInfo);
  view->rootContext()->setContextProperty(QStringLiteral("appSettings"), &appSettings);
  view->rootContext()->setContextProperty(QStringLiteral("osmAccount"), &osmAccount);
  view->rootContext()->setContextProperty(QStringLiteral("bookmarksIO"), &bookmarksIO);
  view->rootContext()->setContextProperty(QStringLiteral("urlHandler"), &urlHandler);
  view->rootContext()->setContextProperty(QStringLiteral("mapsStorage"), &mapsStorage);
  view->rootContext()->setContextProperty(QStringLiteral("downloads"), &downloads);
  view->setSource(SailfishApp::pathToMainQml());
  view->show();
  if (!urls.isEmpty())
    urlHandler.openUrl(urls);

  return app->exec();
}
