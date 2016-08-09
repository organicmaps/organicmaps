#include "qt/preferences_dialog.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/settings.hpp"

#include <QtGui/QIcon>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QCheckBox>
  #include <QtGui/QHBoxLayout>
  #include <QtGui/QVBoxLayout>
  #include <QtGui/QTableWidget>
  #include <QtGui/QHeaderView>
  #include <QtGui/QPushButton>
  #include <QtGui/QGroupBox>
  #include <QtGui/QButtonGroup>
  #include <QtGui/QRadioButton>
#else
  #include <QtWidgets/QCheckBox>
  #include <QtWidgets/QHBoxLayout>
  #include <QtWidgets/QVBoxLayout>
  #include <QtWidgets/QTableWidget>
  #include <QtWidgets/QHeaderView>
  #include <QtWidgets/QPushButton>
  #include <QtWidgets/QGroupBox>
  #include <QtWidgets/QButtonGroup>
  #include <QtWidgets/QRadioButton>
#endif

using namespace measurement_utils;

#ifdef BUILD_DESIGNER
string const kEnabledAutoRegenGeomIndex = "EnabledAutoRegenGeomIndex";
#endif

namespace qt
{
  PreferencesDialog::PreferencesDialog(QWidget * parent)
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
  {
    QIcon icon(":/ui/logo.png");
    setWindowIcon(icon);
    setWindowTitle(tr("Preferences"));

    m_pUnits = new QButtonGroup(this);
    QGroupBox * radioBox = new QGroupBox("System of measurement");
    {
      QHBoxLayout * pLayout = new QHBoxLayout();

      QRadioButton * p = new QRadioButton("Metric");
      pLayout->addWidget(p);
      m_pUnits->addButton(p, static_cast<int>(Units::Metric));

      p = new QRadioButton("Imperial (foot)");
      pLayout->addWidget(p);
      m_pUnits->addButton(p, static_cast<int>(Units::Imperial));

      radioBox->setLayout(pLayout);

      Units u;
      if (!settings::Get(settings::kMeasurementUnits, u))
      {
        // set default measurement from system locale
        if (QLocale::system().measurementSystem() == QLocale::MetricSystem)
          u = Units::Metric;
        else
          u = Units::Imperial;
      }
      m_pUnits->button(static_cast<int>(u))->setChecked(true);

      connect(m_pUnits, SIGNAL(buttonClicked(int)), this, SLOT(OnUnitsChanged(int)));
    }

  #ifdef BUILD_DESIGNER
    QCheckBox * checkBox = new QCheckBox("Enable auto regeneration of geometry index");
    {
      bool enabled = false;
      if (!settings::Get(kEnabledAutoRegenGeomIndex, enabled))
      {
        settings::Set(kEnabledAutoRegenGeomIndex, false);
      }
      checkBox->setChecked(enabled);
      connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(OnEnabledAutoRegenGeomIndex(int)));
    }
  #endif

    QHBoxLayout * bottomLayout = new QHBoxLayout();
    {
      QPushButton * closeButton = new QPushButton(tr("Close"));
      closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      closeButton->setDefault(true);
      connect(closeButton, SIGNAL(clicked()), this, SLOT(OnCloseClick()));

      bottomLayout->addStretch(1);
      bottomLayout->setSpacing(0);
      bottomLayout->addWidget(closeButton);
    }

    QVBoxLayout * finalLayout = new QVBoxLayout();
    finalLayout->addWidget(radioBox);
  #ifdef BUILD_DESIGNER
    finalLayout->addWidget(checkBox);
  #endif
    finalLayout->addLayout(bottomLayout);
    setLayout(finalLayout);
  }

  void PreferencesDialog::OnCloseClick()
  {
    done(0);
  }

  void PreferencesDialog::OnUnitsChanged(int i)
  {
    using namespace settings;

    Units u;
    switch (i)
    {
    case 0: u = Units::Metric; break;
    case 1: u = Units::Imperial; break;
    }

    settings::Set(kMeasurementUnits, u);
  }

#ifdef BUILD_DESIGNER
  void PreferencesDialog::OnEnabledAutoRegenGeomIndex(int i)
  {
    settings::Set(kEnabledAutoRegenGeomIndex, static_cast<bool>(i));
  }
#endif
}
