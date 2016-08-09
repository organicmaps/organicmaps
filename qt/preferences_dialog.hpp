#pragma once

#include "std/string.hpp"

#include <QtWidgets/QApplication>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QDialog>
#else
  #include <QtWidgets/QDialog>
#endif

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
#ifdef BUILD_DESIGNER
    void OnEnabledAutoRegenGeomIndex(int i);
#endif

  private:
    QButtonGroup * m_pUnits;
  };
} // namespace qt

#ifdef BUILD_DESIGNER
extern string const kEnabledAutoRegenGeomIndex;
#endif
