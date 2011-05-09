#pragma once

#include <QtGui/QDialog>

class QTableWidget;

namespace qt
{
  class PreferencesDialog : public QDialog
  {
    Q_OBJECT

    virtual QSize	sizeHint () const { return QSize(400, 400); }
    virtual void done(int);

  public:
    explicit PreferencesDialog(QWidget * parent, bool & autoUpdatesEnabled);

  private slots:
    void OnCloseClick();
    void OnUpClick();
    void OnDownClick();

  private:
    bool & m_autoUpdatesEnabled;
    QTableWidget * m_pTable;
  };
} // namespace qt
