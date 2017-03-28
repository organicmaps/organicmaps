#include "search/search_quality/assessment_tool/samples_view.hpp"

#include "search/search_quality/assessment_tool/helpers.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <QtGui/QStandardItem>
#include <QtWidgets/QHeaderView>

// SamplesView::Model ------------------------------------------------------------------------------
SamplesView::Model::Model(QWidget * parent)
  : QStandardItemModel(0 /* rows */, 1 /* columns */, parent)
{
}

void SamplesView::Model::SetSamples(std::vector<search::Sample> const & samples)
{
  removeRows(0, rowCount());
  for (auto const & sample : samples)
    appendRow(new QStandardItem(ToQString(sample.m_query)));

  m_changed.assign(samples.size(), false);
}

void SamplesView::Model::OnSampleChanged(size_t index, bool hasEdits)
{
  CHECK_LESS(index, m_changed.size(), ());
  m_changed[index] = hasEdits;
}

QVariant SamplesView::Model::data(QModelIndex const & index, int role) const
{
  if (role != Qt::BackgroundRole)
    return QStandardItemModel::data(index, role);

  CHECK_LESS(index.row(), m_changed.size(), ());
  if (m_changed[index.row()])
    return QBrush(QColor(255, 255, 200));
  return QBrush(Qt::transparent);
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
  return selectionModel()->isRowSelected(index, QModelIndex());
}
