#include "testing/testing.hpp"

#include "qt/qt_common/translations.hpp"

#include "platform/localization.hpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QPushButton>

#include <string>
#include <vector>

namespace translations_tests
{
QApplication & App()
{
  static int argc = 0;
  static QApplication app(argc, nullptr);
  return app;
}

UNIT_TEST(Translations_PreferenceOrderAndLocaleVariants)
{
  struct Case
  {
    std::vector<std::string> m_preferred;
    std::string m_language;
  };
  for (auto const & test : std::vector<Case>{{{"zh-Hans-HK"}, "zh-Hans"},
                                             {{"zh-CN-u-fw-mon"}, "zh-Hans"},
                                             {{"zh_HK_#Hans"}, "zh-Hans"},
                                             {{"zh_Hant_TW"}, "zh-Hant"},
                                             {{"de@euro"}, "de"},
                                             {{"pt_BR.UTF-8"}, "pt-BR"},
                                             {{"pt-br"}, "pt-BR"},
                                             {{"en_GB.UTF-8"}, "en-GB"},
                                             {{"fr_CA.UTF-8"}, "fr-CA"},
                                             {{"eo", "fr"}, "fr"},
                                             {{"zha"}, "en"},
                                             {{"C.UTF-8"}, "en"},
                                             {{}, "en"}})
  {
    auto translator = qt::InstallTranslator(App(), test.m_preferred);
    TEST_EQUAL(translator->language().toStdString(), test.m_language, (test.m_preferred));
    TEST_NOT_EQUAL(qt::Tr("download_maps").toStdString(), "download_maps", (test.m_language));
  }
}

UNIT_TEST(Translations_LayoutDirectionFollowsSelectedCatalog)
{
  for (std::string const language : {"ar", "he", "fr", "en"})
  {
    auto translator = qt::InstallTranslator(App(), {&language, 1});
    TEST_EQUAL(static_cast<int>(App().layoutDirection()),
               static_cast<int>(language == "ar" || language == "he" ? Qt::RightToLeft : Qt::LeftToRight), (language));
  }
}

UNIT_TEST(Translations_StandardControlsUseEmbeddedQtCatalog)
{
  struct Case
  {
    std::string m_language;
    std::string m_cancel;
    std::string m_yes;
  };
  for (auto const & test :
       std::vector<Case>{{"fr", "Annuler", "Oui"}, {"en", "Cancel", "Yes"}, {"en-GB", "Cancel", "Yes"}})
  {
    auto translator = qt::InstallTranslator(App(), {&test.m_language, 1});
    QDialogButtonBox buttons(QDialogButtonBox::Cancel | QDialogButtonBox::Yes);
    TEST_EQUAL(buttons.button(QDialogButtonBox::Cancel)->text().toStdString(), test.m_cancel, (test.m_language));
    TEST_EQUAL(buttons.button(QDialogButtonBox::Yes)->text().remove('&').toStdString(), test.m_yes, (test.m_language));
  }
}

UNIT_TEST(Translations_CoreStringsAndFeatureTypesUseTheSelectedCatalog)
{
  std::string const language = "fr";
  {
    auto translator = qt::InstallTranslator(App(), {&language, 1});
    TEST_EQUAL(platform::GetLocalizedString("share_my_position"), "Je suis ici sur Organic Maps", ());
    TEST_EQUAL(platform::GetLocalizedTypeName("amenity-restaurant"), "Restaurant", ());
    TEST_EQUAL(platform::GetLocalizedTypeName("cuisine-italian"), "Cuisine italienne", ());
  }
  // Command-line clients share this backend without installing the UI's catalogs.
  TEST_EQUAL(platform::GetLocalizedTypeName("cuisine-italian"), "cuisine-italian", ());
}

UNIT_TEST(Translations_DataSizeLocalePreservesRegionalPreferences)
{
  struct Case
  {
    std::string m_language;
    char const * m_regionalLocale;
    char const * m_expectedLocale;
  };
  QLocale const defaultLocale;
  auto const number = defaultLocale.toString(1234.5, 'f', 1).toStdString();
  for (auto const & test : std::vector<Case>{{"kk", "kk_KZ", "en_US"},
                                             {"en", "de_DE", "en_US"},
                                             {"de-CH", "de_CH", "de_CH"},
                                             {"fr-CA", "fr_CA", "fr_CA"},
                                             {"ar", "ar_EG", "ar_EG"},
                                             {"zh-Hans-HK", "zh_Hant_HK", "zh_CN"}})
  {
    auto translator = qt::InstallTranslator(App(), {&test.m_language, 1});
    auto const locale = qt::GetDataSizeLocale(*translator, QLocale(test.m_regionalLocale));
    TEST_EQUAL(locale.name().toStdString(), test.m_expectedLocale, (test.m_language));
    TEST(QLocale() == defaultLocale, (test.m_language));
    TEST_EQUAL(QLocale().toString(1234.5, 'f', 1).toStdString(), number, (test.m_language));
  }
}

UNIT_TEST(Translations_DataSizesUseFallbackCatalogLanguage)
{
  std::string const language = "kk";
  auto translator = qt::InstallTranslator(App(), {&language, 1});
  auto const locale = qt::GetDataSizeLocale(*translator, QLocale("kk_KZ"));
  auto const size = qt::Tr("downloader_of")
                        .arg(locale.formattedDataSize(0, 0, QLocale::DataSizeTraditionalFormat),
                             locale.formattedDataSize(13 * 1024 * 1024, 0, QLocale::DataSizeTraditionalFormat));
  TEST_EQUAL(size.toStdString(), "0 bytes of 13 MB", ());
}
}  // namespace translations_tests
