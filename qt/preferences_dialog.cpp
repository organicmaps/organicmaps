#include "qt/preferences_dialog.hpp"

#include "coding/string_utf8_multilang.hpp"
#include "indexer/map_style.hpp"
#include "map/framework.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"
#include "platform/style_utils.hpp"

#include "qt/qt_common/helpers.hpp"

#include <QLocale>
#include <QtGui/QIcon>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
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
  setWindowTitle(tr("Preferences"));

  QButtonGroup * unitsGroup = new QButtonGroup(this);
  QGroupBox * unitsRadioBox = new QGroupBox("System of measurement");
  {
    QHBoxLayout * layout = new QHBoxLayout();

    QRadioButton * radioButton = new QRadioButton("Metric");
    layout->addWidget(radioButton);
    unitsGroup->addButton(radioButton, static_cast<int>(Units::Metric));

    radioButton = new QRadioButton("Imperial (foot)");
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

  QCheckBox * largeFontCheckBox = new QCheckBox("Use larger font on the map");
  {
    largeFontCheckBox->setChecked(framework.LoadLargeFontsSize());
    connect(largeFontCheckBox, &QCheckBox::stateChanged,
            [&framework](int i) { framework.SetLargeFontsSize(static_cast<bool>(i)); });
  }

  QCheckBox * transliterationCheckBox = new QCheckBox("Transliterate to Latin");
  {
    transliterationCheckBox->setChecked(framework.LoadTransliteration());
    connect(transliterationCheckBox, &QCheckBox::stateChanged, [&framework](int i)
    {
      bool const enable = i > 0;
      framework.SaveTransliteration(enable);
      framework.AllowTransliteration(enable);
    });
  }

  QCheckBox * developerModeCheckBox = new QCheckBox("Developer Mode");
  {
    bool developerMode;
    if (settings::Get(settings::kDeveloperMode, developerMode) && developerMode)
      developerModeCheckBox->setChecked(developerMode);
    connect(developerModeCheckBox, &QCheckBox::stateChanged,
            [](int i) { settings::Set(settings::kDeveloperMode, static_cast<bool>(i)); });
  }

  QLabel * mapLanguageLabel = new QLabel("Map Language");
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

  QLabel * bookmarksPlacementLabel = new QLabel("Bookmark's text placement");
  QComboBox * bookmarksPlacementCB = new QComboBox();
  {
    using settings::Placement;

    QStringList lst;
    for (int i = 0; i < static_cast<int>(Placement::Count); ++i)
      lst << QString::fromStdString(ToString(static_cast<Placement>(i)));

    bookmarksPlacementCB->addItems(lst);

    bookmarksPlacementCB->setCurrentText(QString::fromStdString(ToString(Framework::GetBookmarksTextPlacement())));

    connect(bookmarksPlacementCB, &QComboBox::activated,
            [&framework](int index) { framework.SetBookmarksTextPlacement(static_cast<Placement>(index)); });
  }

  QButtonGroup * nightModeGroup = new QButtonGroup(this);
  QGroupBox * nightModeRadioBox = new QGroupBox("Night Mode");
  {
    using namespace style_utils;
    QHBoxLayout * layout = new QHBoxLayout();

    auto const addButton = [&](QString const & text, NightMode mode)
    {
      QRadioButton * button = new QRadioButton(text);
      layout->addWidget(button);
      nightModeGroup->addButton(button, static_cast<int>(mode));
    };

    addButton("Off", NightMode::Off);
    addButton("On", NightMode::On);
    addButton(tr("Follow system"), NightMode::System);

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

      switch (mode)
      {
      case NightMode::Off: framework.ApplyMapStyleForMode(false /* dark */); break;
      case NightMode::On: framework.ApplyMapStyleForMode(true /* dark */); break;
      case NightMode::System: qt::common::ApplySystemNightMode(framework); break;
      }
    });
  }

  QGroupBox * tilesBox = new QGroupBox("Satellite Imagery");
  {
    QVBoxLayout * layout = new QVBoxLayout();

    QLineEdit * urlEdit = new QLineEdit();
    urlEdit->setPlaceholderText("https://xxx.yyy/{z}/{x}/{y}.png");
    urlEdit->setText(QString::fromStdString(framework.GetBackgroundTilesURL()));

    QSpinBox * sizeSpin = new QSpinBox();
    sizeSpin->setRange(1, 1000);
    sizeSpin->setValue(static_cast<int>(framework.GetBackgroundTilesCacheSize()));
    sizeSpin->setSuffix(" MB");

    // Opacity of vector area objects drawn over the imagery: 0 % hides them, 100 % is fully opaque.
    QSpinBox * opacitySpin = new QSpinBox();
    opacitySpin->setRange(0, 100);
    opacitySpin->setValue(static_cast<int>(framework.GetBackgroundTilesAreaOpacity()));
    opacitySpin->setSuffix(" %");

    QFormLayout * form = new QFormLayout();
    form->addRow("Server URL", urlEdit);
    form->addRow("Cache size", sizeSpin);
    form->addRow("Area objects opacity", opacitySpin);

    QCheckBox * enableCheckBox = new QCheckBox("Satellite Imagery");
    enableCheckBox->setChecked(framework.IsBackgroundTilesEnabled());
    auto const updateEnabled = [urlEdit, sizeSpin, opacitySpin](bool en)
    {
      urlEdit->setEnabled(en);
      sizeSpin->setEnabled(en);
      opacitySpin->setEnabled(en);
    };
    updateEnabled(enableCheckBox->isChecked());
    // Only update the field availability live; the values are applied once when the dialog closes.
    connect(enableCheckBox, &QCheckBox::stateChanged, [updateEnabled](int i) { updateEnabled(i > 0); });

    // Apply all tile settings together when the Preferences dialog is closed.
    connect(this, &QDialog::finished, [&framework, enableCheckBox, urlEdit, sizeSpin, opacitySpin](int)
    {
      framework.SetBackgroundTiles(enableCheckBox->isChecked(), urlEdit->text().toStdString(),
                                   static_cast<uint32_t>(sizeSpin->value()),
                                   static_cast<uint32_t>(opacitySpin->value()));
    });

    QLabel * disclaimer = new QLabel(
        "Custom tile sources are provided by you. Use only services you are allowed to access. You are "
        "responsible for attribution, license terms, API keys, quotas, and usage limits.");
    disclaimer->setWordWrap(true);

    layout->addWidget(enableCheckBox);
    layout->addLayout(form);
    layout->addWidget(disclaimer);
    tilesBox->setLayout(layout);
  }

#ifdef BUILD_DESIGNER
  QCheckBox * indexRegenCheckBox = new QCheckBox("Enable auto regeneration of geometry index");
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
    QPushButton * closeButton = new QPushButton(tr("Close"));
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
  finalLayout->addWidget(tilesBox);
#ifdef BUILD_DESIGNER
  finalLayout->addWidget(indexRegenCheckBox);
#endif
  finalLayout->addLayout(bottomLayout);
  setLayout(finalLayout);
}
}  // namespace qt
