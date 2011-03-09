#include "preferences_dialog.hpp"

#include <QtGui/QIcon>
#include <QtGui/QCheckBox>
#include <QtGui/QHBoxLayout>

namespace qt
{
  PreferencesDialog::PreferencesDialog(QWidget * parent, bool & autoUpdatesEnabled)
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint), m_autoUpdatesEnabled(autoUpdatesEnabled)
  {
    QIcon icon(":logo.png");
    setWindowIcon(icon);
    setWindowTitle(tr("Preferences"));

    QCheckBox * updateCheckbox = new QCheckBox("Automatically check for updates on startup", this);
    updateCheckbox->setCheckState(autoUpdatesEnabled ? Qt::Checked : Qt::Unchecked);
    connect(updateCheckbox, SIGNAL(stateChanged(int)), this, SLOT(OnCheckboxStateChanged(int)));

    QHBoxLayout * hBox = new QHBoxLayout();
    hBox->addWidget(updateCheckbox);
    setLayout(hBox);
  }

  void PreferencesDialog::OnCheckboxStateChanged(int state)
  {
    m_autoUpdatesEnabled = (state == Qt::Checked) ? true : false;
  }
}
