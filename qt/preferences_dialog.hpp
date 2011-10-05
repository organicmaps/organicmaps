#pragma once

#include <QtGui/QDialog>

class QTableWidget;
class QButtonGroup;

namespace qt
{
  class PreferencesDialog : public QDialog
  {
    typedef QDialog base_t;

    Q_OBJECT

    virtual QSize	sizeHint () const { return QSize(400, 400); }
    virtual void done(int);

  public:
    explicit PreferencesDialog(QWidget * parent);

  private slots:
    void OnCloseClick();
    void OnUpClick();
    void OnDownClick();
    void OnUnitsChanged(int i);

  private:
    QTableWidget * m_pTable;
    QButtonGroup * m_pUnits;
  };
} // namespace qt
