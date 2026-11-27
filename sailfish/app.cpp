#include "sailfish/app_settings.hpp"
#include "sailfish/framework_access.hpp"
#include "sailfish/map_item.hpp"
#include "sailfish/url_handler.hpp"

#include "qt/qt_common/translations.hpp"

#include "map/framework.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "base/logging.hpp"

#include <qqml.h>
#include <QDir>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickView>
#include <QStandardPaths>
#include <QSurfaceFormat>

#include <clocale>
#include <cstdlib>
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

void SetupPlatform()
{
  // Platform reads these on first use; the defaults match the installed RPM layout and Sailjail.
  SetEnvIfUnset("MWM_RESOURCES_DIR", SailfishApp::pathTo(QStringLiteral("data")).toLocalFile());
  QString const writableDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(writableDir);
  SetEnvIfUnset("MWM_WRITABLE_DIR", writableDir);

  LOG(LINFO, ("Resources:", GetPlatform().ResourcesDir(), "Writable:", GetPlatform().WritableDir(),
              "Settings:", GetPlatform().SettingsDir()));
}

void RegisterQmlTypes()
{
  qmlRegisterType<sailfish::MapItem>("app.organicmaps", 1, 0, "MapItem");
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
  switch (dbusInbox.Claim())
  {
  case sailfish::DBusInbox::ClaimResult::Owner: break;
  case sailfish::DBusInbox::ClaimResult::OtherInstance:
    return sailfish::DBusInbox::OpenInRunningApp(urls) ? EXIT_SUCCESS : EXIT_FAILURE;
  case sailfish::DBusInbox::ClaimResult::Failed: return EXIT_FAILURE;
  }

  SetupPlatform();

  auto const & preferred = languages::GetSystemPreferred();
  auto const translator = qt::InstallTranslator(*app, {preferred.data(), preferred.size()});

  Framework framework;
  framework.SetupMeasurementSystem();
  g_framework = &framework;

  RegisterQmlTypes();

  // Declared before the view, which uses them until it is destroyed.
  sailfish::AppSettings appSettings(framework);
  sailfish::UrlHandler urlHandler(framework);
  dbusInbox.SetHandler(urlHandler);
  std::unique_ptr<QQuickView> view(SailfishApp::createView());
  view->rootContext()->setContextProperty(QStringLiteral("appSettings"), &appSettings);
  view->rootContext()->setContextProperty(QStringLiteral("urlHandler"), &urlHandler);
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
