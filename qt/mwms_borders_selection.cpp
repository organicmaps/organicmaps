#include "qt/mwms_borders_selection.hpp"

#include "base/assert.hpp"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QVBoxLayout>

namespace qt
{
MwmsBordersSelection::MwmsBordersSelection(QWidget * parent)
  : QDialog(parent),
    m_form(this)
{
  setWindowTitle("Mwms borders selection settings");

  m_radioWithPoints = new QRadioButton(tr("Show borders with points."));
  m_radioJustBorders = new QRadioButton(tr("Show just borders."));

  m_radioJustBorders->setChecked(true);

  auto * vbox = new QVBoxLayout;
  vbox->addWidget(m_radioWithPoints);
  vbox->addWidget(m_radioJustBorders);

  m_form.addRow(vbox);
  AddButtonBox();
}

void MwmsBordersSelection::AddButtonBox()
{
  auto * buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

  m_form.addRow(buttonBox);

  QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

MwmsBordersSelection::Response MwmsBordersSelection::ShowModal()
{
  if (exec() != QDialog::Accepted)
    return Response::Canceled;

  if (m_radioJustBorders->isChecked())
    return Response::JustBorders;

  if (m_radioWithPoints->isChecked())
    return Response::WithPointsAndBorders;

  UNREACHABLE();
}
}  // namespace qt
