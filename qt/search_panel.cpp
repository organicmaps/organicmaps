#include "search_panel.hpp"
#include "draw_widget.hpp"

#include "../search/result.hpp"

#include "../std/bind.hpp"

#include <QtGui/QHeaderView>
#include <QtGui/QTableWidget>
#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>

namespace qt
{

SearchPanel::SearchPanel(DrawWidget * drawWidget, QWidget * parent)
  : QWidget(parent), m_pDrawWidget(drawWidget)
{
  m_pEditor = new QLineEdit(this);
  connect(m_pEditor, SIGNAL(textChanged(QString const &)), this, SLOT(OnSearchTextChanged(QString const &)));

  m_pTable = new QTableWidget(0, 2, this);
  m_pTable->setAlternatingRowColors(true);
  m_pTable->setShowGrid(false);
  m_pTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_pTable->verticalHeader()->setVisible(false);
  m_pTable->horizontalHeader()->setVisible(false);
  m_pTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  connect(m_pTable, SIGNAL(cellClicked(int,int)), this, SLOT(OnSearchPanelItemClicked(int,int)));

  QVBoxLayout * verticalLayout = new QVBoxLayout();
  verticalLayout->addWidget(m_pEditor);
  verticalLayout->addWidget(m_pTable);
  setLayout(verticalLayout);
}

void SearchPanel::OnSearchResult(search::Result const & result)
{
  if (!result.GetString().empty())  // last element
  {
    int const rowCount = m_pTable->rowCount();

    m_pTable->setRowCount(rowCount + 1);
    QTableWidgetItem * item = new QTableWidgetItem(QString::fromUtf8(result.GetString().c_str()));
    item->setData(Qt::UserRole, QRectF(QPointF(result.GetFeatureRect().minX(),
                                               result.GetFeatureRect().maxY()),
                                       QPointF(result.GetFeatureRect().maxX(),
                                               result.GetFeatureRect().minY())));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_pTable->setItem(rowCount, 0, item);
  }
}

void SearchPanel::OnSearchTextChanged(QString const & str)
{
  // clear old results
  m_pTable->clear();
  m_pTable->setRowCount(0);

  QString const normalized = str.normalized(QString::NormalizationForm_KC);
  if (!normalized.isEmpty())
    m_pDrawWidget->Search(normalized.toUtf8().constData(),
                        bind(&SearchPanel::OnSearchResult, this, _1));
}

void SearchPanel::OnSearchPanelItemClicked(int row, int)
{
  // center viewport on clicked item
  QRectF const rect = m_pTable->item(row, 0)->data(Qt::UserRole).toRectF();
  m2::RectD const r2(rect.left(), rect.bottom(), rect.right(), rect.top());
  m_pDrawWidget->ShowFeature(r2);
}

void SearchPanel::showEvent(QShowEvent *)
{
  m_pEditor->setFocus();
}

void SearchPanel::hideEvent(QHideEvent *)
{
  m_pEditor->clearFocus();
}

}
