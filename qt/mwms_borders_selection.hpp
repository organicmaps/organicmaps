#pragma once

#include "qt/selection.hpp"

#include <string>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QRadioButton>

class Framework;

namespace qt
{
class MwmsBordersSelection : public QDialog
{
public:
  MwmsBordersSelection(QWidget * parent);

  SelectionMode ShowModal();

private:
  QGroupBox * CreateSourceChoosingGroup();
  QGroupBox * CreateViewTypeGroup();
  QGroupBox * CreateButtonBoxGroup();

  QRadioButton * m_radioBordersFromPackedPolygon;
  QRadioButton * m_radioBordersFromData;

  QRadioButton * m_radioWithPoints;
  QRadioButton * m_radioJustBorders;
  QRadioButton * m_radioBoundingBox;
};
}  // namespace qt
