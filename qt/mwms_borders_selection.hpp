#pragma once

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QRadioButton>

#include <string>

class Framework;

namespace qt
{
class MwmsBordersSelection : public QDialog
{
public:
  MwmsBordersSelection(QWidget * parent);

  enum class Response
  {
    JustBorders,
    WithPointsAndBorders,

    Canceled
  };

  Response ShowModal();

private:
  void AddButtonBox();

  QFormLayout m_form;
  QRadioButton * m_radioWithPoints;
  QRadioButton * m_radioJustBorders;
};
}  // namespace qt
