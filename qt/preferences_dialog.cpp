#include "qt/preferences_dialog.hpp"

#include "coding/string_utf8_multilang.hpp"
#include "indexer/map_style.hpp"
#include "map/framework.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"
#include "platform/style_utils.hpp"

#include "qt/qt_common/helpers.hpp"
#include "qt/qt_common/translations.hpp"

#include <QLocale>
#include <QtGui/QIcon>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QVBoxLayout>

using namespace measurement_utils;

#ifdef BUILD_DESIGNER
std::string const kEnabledAutoRegenGeomIndex = "EnabledAutoRegenGeomIndex";
#endif

namespace qt
{
PreferencesDialog::PreferencesDialog(QWidget * parent, Framework & framework)
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
  QIcon icon(":/ui/logo.png");
  setWindowIcon(icon);
  setWindowTitle(Tr("desktop_preferences"));

  QButtonGroup * unitsGroup = new QButtonGroup(this);
  QGroupBox * unitsRadioBox = new QGroupBox(Tr("desktop_system_of_measurement"));
  {
    QHBoxLayout * layout = new QHBoxLayout();

    QRadioButton * radioButton = new QRadioButton(Tr("desktop_metric"));
    layout->addWidget(radioButton);
    unitsGroup->addButton(radioButton, static_cast<int>(Units::Metric));

    radioButton = new QRadioButton(Tr("desktop_imperial_foot"));
    layout->addWidget(radioButton);
    unitsGroup->addButton(radioButton, static_cast<int>(Units::Imperial));

    unitsRadioBox->setLayout(layout);

    Units u;
    if (!settings::Get(settings::kMeasurementUnits, u))
    {
      // Set default measurement from system locale
      if (QLocale::system().measurementSystem() == QLocale::MetricSystem)
        u = Units::Metric;
      else
        u = Units::Imperial;
    }
    unitsGroup->button(static_cast<int>(u))->setChecked(true);

    // Temporary to pass the address of overloaded function.
    void (QButtonGroup::*buttonClicked)(int) = &QButtonGroup::idClicked;
    connect(unitsGroup, buttonClicked, [&framework](int i)
    {
      Units u = Units::Metric;
      switch (i)
      {
      case 0: u = Units::Metric; break;
      case 1: u = Units::Imperial; break;
      }

      settings::Set(settings::kMeasurementUnits, u);
      framework.SetupMeasurementSystem();
    });
  }

  QCheckBox * largeFontCheckBox = new QCheckBox(Tr("desktop_use_larger_font_on_map"));
  {
    largeFontCheckBox->setChecked(framework.LoadLargeFontsSize());
    connect(largeFontCheckBox, &QCheckBox::stateChanged,
            [&framework](int i) { framework.SetLargeFontsSize(static_cast<bool>(i)); });
  }

  QCheckBox * transliterationCheckBox = new QCheckBox(Tr("desktop_transliterate_to_latin"));
  {
    transliterationCheckBox->setChecked(framework.LoadTransliteration());
    connect(transliterationCheckBox, &QCheckBox::stateChanged, [&framework](int i)
    {
      bool const enable = i > 0;
      framework.SaveTransliteration(enable);
      framework.AllowTransliteration(enable);
    });
  }

  QCheckBox * developerModeCheckBox = new QCheckBox(Tr("desktop_developer_mode"));
  {
    bool developerMode;
    if (settings::Get(settings::kDeveloperMode, developerMode) && developerMode)
      developerModeCheckBox->setChecked(developerMode);
    connect(developerModeCheckBox, &QCheckBox::stateChanged,
            [](int i) { settings::Set(settings::kDeveloperMode, static_cast<bool>(i)); });
  }

  QLabel * mapLanguageLabel = new QLabel(Tr("change_map_locale"));
  QComboBox * mapLanguageComboBox = new QComboBox();
  {
    // The property maxVisibleItems is ignored for non-editable comboboxes in styles that
    // return true for `QStyle::SH_ComboBox_Popup such as the Mac style or the Gtk+ Style.
    // So we ensure that it returns false here.
    mapLanguageComboBox->setStyleSheet("QComboBox { combobox-popup: 0; }");
    mapLanguageComboBox->setMaxVisibleItems(10);
    StringUtf8Multilang::Languages const & supportedLanguages =
        StringUtf8Multilang::GetSupportedLanguages(/* includeServiceLangs */ false);
    QStringList languagesList;
    for (auto const & language : supportedLanguages)
      languagesList << QString::fromStdString(std::string(language.m_name));

    mapLanguageComboBox->addItems(languagesList);
    std::string const & mapLanguageCode = framework.GetMapLanguageCode();
    int8_t languageIndex = StringUtf8Multilang::GetLangIndex(mapLanguageCode);
    if (languageIndex == StringUtf8Multilang::kUnsupportedLanguageCode)
      languageIndex = StringUtf8Multilang::kDefaultCode;

    mapLanguageComboBox->setCurrentText(
        QString::fromStdString(std::string(StringUtf8Multilang::GetLangNameByCode(languageIndex))));
    connect(mapLanguageComboBox, &QComboBox::activated, [&framework, &supportedLanguages](int index)
    {
      auto const & mapLanguageCode = std::string(supportedLanguages[index].m_code);
      framework.SetMapLanguageCode(mapLanguageCode);
    });
  }

  QLabel * bookmarksPlacementLabel = new QLabel(Tr("bookmarks_text_placement_title"));
  QComboBox * bookmarksPlacementCB = new QComboBox();
  {
    using settings::Placement;

    bookmarksPlacementCB->addItem(Tr("off"));
    bookmarksPlacementCB->addItem(Tr("show_to_the_right"));
    bookmarksPlacementCB->addItem(Tr("show_at_the_bottom"));
    bookmarksPlacementCB->setCurrentIndex(static_cast<int>(Framework::GetBookmarksTextPlacement()));

    connect(bookmarksPlacementCB, &QComboBox::activated,
            [&framework](int index) { framework.SetBookmarksTextPlacement(static_cast<Placement>(index)); });
  }

  QButtonGroup * nightModeGroup = new QButtonGroup(this);
  QGroupBox * nightModeRadioBox = new QGroupBox(Tr("desktop_night_mode"));
  {
    using namespace style_utils;
    QHBoxLayout * layout = new QHBoxLayout();

    auto const addButton = [&](QString const & text, NightMode mode)
    {
      QRadioButton * button = new QRadioButton(text);
      layout->addWidget(button);
      nightModeGroup->addButton(button, static_cast<int>(mode));
    };

    addButton(Tr("off"), NightMode::Off);
    addButton(Tr("on"), NightMode::On);
    addButton(Tr("follow_system"), NightMode::System);

    nightModeRadioBox->setLayout(layout);

    NightMode nightModeSetting = GetNightModeSetting();
    if (nightModeSetting == NightMode::Off && MapStyleIsDark(framework.GetMapStyle()))
      nightModeSetting = NightMode::On;

    if (QAbstractButton * button = nightModeGroup->button(static_cast<int>(nightModeSetting)))
      button->setChecked(true);

    void (QButtonGroup::*buttonClicked)(int) = &QButtonGroup::idClicked;
    connect(nightModeGroup, buttonClicked, [&framework](int i)
    {
      using namespace style_utils;
      if (i < static_cast<int>(NightMode::Off) || i > static_cast<int>(NightMode::System))
        return;

      auto const mode = static_cast<NightMode>(i);
      SetNightModeSetting(mode);

      auto const currStyle = framework.GetMapStyle();
      switch (mode)
      {
      case NightMode::Off: framework.SetMapStyle(GetLightMapStyleVariant(currStyle)); break;
      case NightMode::On: framework.SetMapStyle(GetDarkMapStyleVariant(currStyle)); break;
      case NightMode::System: qt::common::ApplySystemNightMode(framework); break;
      }
    });
  }

#ifdef BUILD_DESIGNER
  QCheckBox * indexRegenCheckBox = new QCheckBox(Tr("desktop_enable_auto_regeneration_geometry_index"));
  {
    bool enabled = false;
    if (!settings::Get(kEnabledAutoRegenGeomIndex, enabled))
      settings::Set(kEnabledAutoRegenGeomIndex, false);
    indexRegenCheckBox->setChecked(enabled);
    connect(indexRegenCheckBox, &QCheckBox::stateChanged,
            [](int i) { settings::Set(kEnabledAutoRegenGeomIndex, static_cast<bool>(i)) });
  }
#endif

  QHBoxLayout * bottomLayout = new QHBoxLayout();
  {
    QPushButton * closeButton = new QPushButton(Tr("close"));
    closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    closeButton->setDefault(true);
    connect(closeButton, &QAbstractButton::clicked, [this]() { done(0); });

    bottomLayout->addStretch(1);
    bottomLayout->setSpacing(0);
    bottomLayout->addWidget(closeButton);
  }

  QVBoxLayout * finalLayout = new QVBoxLayout();
  finalLayout->addWidget(unitsRadioBox);
  finalLayout->addWidget(largeFontCheckBox);
  finalLayout->addWidget(transliterationCheckBox);
  finalLayout->addWidget(developerModeCheckBox);
  finalLayout->addWidget(mapLanguageLabel);
  finalLayout->addWidget(mapLanguageComboBox);
  finalLayout->addWidget(bookmarksPlacementLabel);
  finalLayout->addWidget(bookmarksPlacementCB);
  finalLayout->addWidget(nightModeRadioBox);
#ifdef BUILD_DESIGNER
  finalLayout->addWidget(indexRegenCheckBox);
#endif
  finalLayout->addLayout(bottomLayout);
  setLayout(finalLayout);
}
}  // namespace qt
