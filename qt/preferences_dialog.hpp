#pragma once

#include <QtWidgets/QDialog>

class QTableWidget;
class QButtonGroup;

namespace qt
{
  class PreferencesDialog : public QDialog
  {
    typedef QDialog base_t;

    Q_OBJECT

    virtual QSize	sizeHint () const { return QSize(400, 400); }

  public:
    explicit PreferencesDialog(QWidget * parent);

  private slots:
    void OnCloseClick();
    void OnUnitsChanged(int i);

  private:
    QButtonGroup * m_pUnits;
  };
} // namespace qt
