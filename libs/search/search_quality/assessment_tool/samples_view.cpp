#include "search/search_quality/assessment_tool/samples_view.hpp"

#include "search/search_quality/assessment_tool/helpers.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <string>

#include <QtGui/QAction>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QStandardItem>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMenu>

// SamplesView::Model ------------------------------------------------------------------------------
SamplesView::Model::Model(QWidget * parent) : QStandardItemModel(0 /* rows */, 1 /* columns */, parent) {}

QVariant SamplesView::Model::data(QModelIndex const & index, int role) const
{
  auto const row = index.row();

  if (role == Qt::DisplayRole && m_samples.IsValid())
    return QString::fromStdString(m_samples.GetLabel(row));
  if (role == Qt::BackgroundRole && m_samples.IsValid())
  {
    if (m_samples.IsChanged(row))
      return QBrush(QColor(0xFF, 0xFF, 0xC8));

    if (m_samples.GetSearchState(row) == Context::SearchState::InQueue)
      return QBrush(QColor(0xFF, 0xCC, 0x66));

    if (m_samples.GetSearchState(row) == Context::SearchState::Completed)
      return QBrush(QColor(0xCA, 0xFE, 0xDB));

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
  // TODO: Do not invoke virtual functions from constructor.
  setModel(m_model);
}

bool SamplesView::IsSelected(size_t index) const
{
  return selectionModel()->isRowSelected(base::checked_cast<int>(index), QModelIndex());
}

void SamplesView::contextMenuEvent(QContextMenuEvent * event)
{
  QModelIndex modelIndex = selectionModel()->currentIndex();
  if (!modelIndex.isValid())
    return;

  int const index = modelIndex.row();
  bool const isUseless = m_model->SampleIsUseless(index);

  QMenu menu(this);
  auto const text = std::string(isUseless ? "unmark" : "mark") + " sample as useless";
  auto const * action = menu.addAction(text.c_str());
  connect(action, &QAction::triggered, [this, index]() { emit FlipSampleUsefulness(index); });
  menu.exec(event->globalPos());
}
