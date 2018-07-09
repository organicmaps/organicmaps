#include "search/search_quality/assessment_tool/samples_view.hpp"

#include "search/search_quality/assessment_tool/helpers.hpp"

#include "base/assert.hpp"

#include <QtGui/QStandardItem>
#include <QtWidgets/QHeaderView>

// SamplesView::Model ------------------------------------------------------------------------------
SamplesView::Model::Model(QWidget * parent)
  : QStandardItemModel(0 /* rows */, 1 /* columns */, parent)
{
}

QVariant SamplesView::Model::data(QModelIndex const & index, int role) const
{
  auto const row = index.row();

  if (role == Qt::DisplayRole && m_samples.IsValid())
    return QString::fromStdString(m_samples.GetLabel(row));
  if (role == Qt::BackgroundRole && m_samples.IsValid())
  {
    if (m_samples.IsChanged(row))
      return QBrush(QColor(255, 255, 200));
    return QBrush(Qt::transparent);
  }
  return QStandardItemModel::data(index, role);
}

// SamplesView -------------------------------------------------------------------------------------
SamplesView::SamplesView(QWidget * parent) : QTableView(parent)
{
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSelectionMode(QAbstractItemView::SingleSelection);

  {
    auto * header = horizontalHeader();
    header->setStretchLastSection(true /* stretch */);
    header->hide();
  }

  m_model = new Model(this /* parent */);
  setModel(m_model);
}

bool SamplesView::IsSelected(size_t index) const
{
  return selectionModel()->isRowSelected(base::checked_cast<int>(index), QModelIndex());
}
