#include "qt/mwms_borders_selection.hpp"

#include "base/assert.hpp"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QSplitter>

namespace qt
{
MwmsBordersSelection::MwmsBordersSelection(QWidget * parent)
  : QDialog(parent)
{
  setWindowTitle("Mwms borders selection settings");

  auto * grid = new QGridLayout;
  grid->addWidget(CreateSourceChoosingGroup(), 0, 0);
  grid->addWidget(CreateViewTypeGroup(), 1, 0);
  grid->addWidget(CreateButtonBoxGroup(), 2, 0);

  setLayout(grid);
}

QGroupBox * MwmsBordersSelection::CreateButtonBoxGroup()
{
  auto * groupBox = new QGroupBox();

  auto * buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

  auto * vbox = new QVBoxLayout;

  vbox->addWidget(buttonBox);
  groupBox->setLayout(vbox);
  groupBox->setFlat(true);

  QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  return groupBox;
}

MwmsBordersSelection::Response MwmsBordersSelection::ShowModal()
{
  if (exec() != QDialog::Accepted)
    return Response::Cancelled;

  if (m_radioBordersFromData->isChecked())
  {
    if (m_radioJustBorders->isChecked())
      return Response::JustBordersByPolyFiles;

    if (m_radioWithPoints->isChecked())
      return Response::WithPointsAndBordersByPolyFiles;

    UNREACHABLE();
  }

  if (m_radioBordersFromPackedPolygon->isChecked())
  {
    if (m_radioJustBorders->isChecked())
      return Response::JustBordersByPackedPolygon;

    if (m_radioWithPoints->isChecked())
      return Response::WithPointsAndBordersByPackedPolygon;

    UNREACHABLE();
  }

  UNREACHABLE();
}

QGroupBox * MwmsBordersSelection::CreateSourceChoosingGroup()
{
  auto * groupBox = new QGroupBox();

  m_radioBordersFromPackedPolygon = new QRadioButton(tr("Show borders from packed_polygon.bin."));
  m_radioBordersFromData = new QRadioButton(tr("Show borders from *.poly files."));

  m_radioBordersFromPackedPolygon->setChecked(true);

  auto * vbox = new QVBoxLayout;

  vbox->addWidget(m_radioBordersFromPackedPolygon);
  vbox->addWidget(m_radioBordersFromData);
  groupBox->setLayout(vbox);

  return groupBox;
}

QGroupBox * MwmsBordersSelection::CreateViewTypeGroup()
{
  auto * groupBox = new QGroupBox();

  m_radioWithPoints = new QRadioButton(tr("Show borders with points."));
  m_radioJustBorders = new QRadioButton(tr("Show just borders."));

  m_radioWithPoints->setChecked(true);

  auto * vbox = new QVBoxLayout;

  vbox->addWidget(m_radioWithPoints);
  vbox->addWidget(m_radioJustBorders);
  groupBox->setLayout(vbox);

  return groupBox;
}
}  // namespace qt
