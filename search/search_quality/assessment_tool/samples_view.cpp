#include "search/search_quality/assessment_tool/samples_view.hpp"

#include "search/search_quality/assessment_tool/helpers.hpp"

#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QHeaderView>

SamplesView::SamplesView(QWidget * parent) : QTableView(parent)
{
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSelectionMode(QAbstractItemView::SingleSelection);

  {
    auto * header = horizontalHeader();
    header->setStretchLastSection(true /* stretch */);
    header->hide();
  }

  m_model = new QStandardItemModel(0 /* rows */, 1 /* columns */, this /* parent */);
  setModel(m_model);
}

void SamplesView::SetSamples(std::vector<search::Sample> const & samples)
{
  m_model->removeRows(0, m_model->rowCount());
  for (auto const & sample : samples)
    m_model->appendRow(new QStandardItem(ToQString(sample.m_query)));
}
