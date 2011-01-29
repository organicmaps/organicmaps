#pragma once

#include <QtGui/QDialog>

namespace qt
{
  class PreferencesDialog : public QDialog
  {
    Q_OBJECT

  public:
    explicit PreferencesDialog(QWidget * parent, bool & autoUpdatesEnabled);

  private slots:
    void OnCheckboxStateChanged(int state);

  private:
    bool & m_autoUpdatesEnabled;
  };
} // namespace qt
