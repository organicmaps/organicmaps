#include "qt/preferences_dialog.hpp"

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

      using namespace Settings;

      QRadioButton * p = new QRadioButton("Metric");
      pLayout->addWidget(p);
      m_pUnits->addButton(p, Metric);

      p = new QRadioButton("Imperial (foot)");
      pLayout->addWidget(p);
      m_pUnits->addButton(p, Foot);

      radioBox->setLayout(pLayout);

      Units u;
      if (!Settings::Get("Units", u))
      {
        // set default measurement from system locale
        if (QLocale::system().measurementSystem() == QLocale::MetricSystem)
          u = Metric;
        else
          u = Foot;
      }
      m_pUnits->button(static_cast<int>(u))->setChecked(true);

      connect(m_pUnits, SIGNAL(buttonClicked(int)), this, SLOT(OnUnitsChanged(int)));
    }


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
    finalLayout->addLayout(bottomLayout);
    setLayout(finalLayout);
  }

  void PreferencesDialog::OnCloseClick()
  {
    done(0);
  }

  void PreferencesDialog::OnUnitsChanged(int i)
  {
    using namespace Settings;

    Units u;
    switch (i)
    {
    case 0: u = Metric; break;
    case 1: u = Foot; break;
    }

    Settings::Set("Units", u);
  }
}
