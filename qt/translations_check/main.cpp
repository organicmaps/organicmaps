#include "qt/qt_common/translations.hpp"

#include "platform/localization.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <QtGui/QGuiApplication>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>

#include <memory>
#include <string>

// Qt 5 builds have no tests yet, so this checks the shared catalogs there.
int main(int argc, char ** argv)
{
  QGuiApplication app(argc, argv);

  struct Case
  {
    std::string m_preferred;
    std::string m_catalog;
    std::string m_downloadMaps;
    std::string m_italian;
  };
  for (auto const & test :
       {Case{"fr", "fr", "Télécharger des cartes", "Cuisine italienne"}, Case{"xx", "en", "Download Maps", "Italian"},
        Case{"pt_BR.UTF-8", "pt-BR", "Baixar mapas", "Cozinha italiana"},
        Case{"zh_Hant_TW", "zh-Hant", "下載地圖", "義大利料理"}, Case{"zh-Hans-HK", "zh-Hans", "下载地图", "意大利菜"},
        Case{"ar", "ar", "تنزيل الخرائط", "مطبخ إيطالي"}})
  {
    auto const translator = qt::InstallTranslator(app, {&test.m_preferred, 1});
    CHECK_EQUAL(translator->objectName().toStdString(), test.m_catalog, ());
    CHECK_EQUAL(platform::GetLocalizedString("download_maps"), test.m_downloadMaps, (test.m_catalog));
    CHECK_EQUAL(platform::GetLocalizedTypeName("cuisine-italian"), test.m_italian, (test.m_catalog));
    auto const direction = test.m_catalog == "ar" ? Qt::RightToLeft : Qt::LeftToRight;
    CHECK(app.layoutDirection() == direction, (test.m_catalog));
  }

  // The Sailfish OS app looks the keys up in QML.
  std::string const french = "fr";
  auto const translator = qt::InstallTranslator(app, {&french, 1});
  QQmlEngine engine;
  QQmlComponent component(&engine);
  component.setData("import QtQml 2.0\nQtObject { property string text: qsTrId(\"layers_title\") }", QUrl());
  std::unique_ptr<QObject> const object(component.create());
  CHECK(object, (component.errorString().toStdString()));
  CHECK_EQUAL(object->property("text").toString().toStdString(), "Styles et couches de cartes", ());
  LOG(LINFO, ("Qt translations OK"));
  return 0;
}
