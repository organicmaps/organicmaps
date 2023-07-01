#include "qt/preferences_dialog.hpp"

#include "map/framework.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include <QtGlobal>  // QT_VERSION_CHECK
#include <QtGui/QIcon>
#include <QLocale>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QRadioButton>

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
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
      void (QButtonGroup::* buttonClicked)(int) = &QButtonGroup::buttonClicked;
#else
      void (QButtonGroup::* buttonClicked)(int) = &QButtonGroup::idClicked;
#endif
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

#ifdef BUILD_DESIGNER
    QCheckBox * indexRegenCheckBox = new QCheckBox("Enable auto regeneration of geometry index");
    {
      bool enabled = false;
      if (!settings::Get(kEnabledAutoRegenGeomIndex, enabled))
        settings::Set(kEnabledAutoRegenGeomIndex, false);
      indexRegenCheckBox->setChecked(enabled);
      connect(indexRegenCheckBox, &QCheckBox::stateChanged, [](int i)
      {
        settings::Set(kEnabledAutoRegenGeomIndex, static_cast<bool>(i))
      });
    }
#endif

    QHBoxLayout * bottomLayout = new QHBoxLayout();
    {
      QPushButton * closeButton = new QPushButton(tr("Close"));
      closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      closeButton->setDefault(true);
      connect(closeButton, &QAbstractButton::clicked, [this](){ done(0); });

      bottomLayout->addStretch(1);
      bottomLayout->setSpacing(0);
      bottomLayout->addWidget(closeButton);
    }

    QVBoxLayout * finalLayout = new QVBoxLayout();
    finalLayout->addWidget(unitsRadioBox);
    finalLayout->addWidget(largeFontCheckBox);
#ifdef BUILD_DESIGNER
    finalLayout->addWidget(indexRegenCheckBox);
#endif
    finalLayout->addLayout(bottomLayout);
    setLayout(finalLayout);
  }
}
