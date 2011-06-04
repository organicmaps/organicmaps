#include "search_panel.hpp"
#include "draw_widget.hpp"

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

  // for multithreading support
  connect(this, SIGNAL(SearchResultSignal(search::Result *)),
          this, SLOT(OnSearchResult(search::Result *)), Qt::QueuedConnection);

  setFocusPolicy(Qt::StrongFocus);
  setFocusProxy(m_pEditor);
}

template<class T> static void ClearVector(vector<T *> & v)
{
  for(size_t i = 0; i < v.size(); ++i)
    delete v[i];
  v.clear();
}

SearchPanel::~SearchPanel()
{
  ClearVector(m_results);
}

void SearchPanel::SearchResultThreadFunc(search::Result const & result)
{
  emit SearchResultSignal(new search::Result(result));
}

void SearchPanel::OnSearchResult(search::Result * result)
{
  if (!result->GetString().empty())  // last element
  {
    int const rowCount = m_pTable->rowCount();
    m_pTable->setRowCount(rowCount + 1);
    QTableWidgetItem * item = new QTableWidgetItem(QString::fromUtf8(result->GetString().c_str()));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_pTable->setItem(rowCount, 0, item);
    m_results.push_back(result);
  }
  else
    delete result;
}

void SearchPanel::OnSearchTextChanged(QString const & str)
{
  // clear old results
  m_pTable->clear();
  m_pTable->setRowCount(0);
  ClearVector(m_results);

  QString const normalized = str.normalized(QString::NormalizationForm_KC);
  if (!normalized.isEmpty())
    m_pDrawWidget->Search(normalized.toUtf8().constData(),
                        bind(&SearchPanel::SearchResultThreadFunc, this, _1));
}

void SearchPanel::OnSearchPanelItemClicked(int row, int)
{
  ASSERT_EQUAL(m_results.size(), static_cast<size_t>(m_pTable->rowCount()), ());
  if (m_results[row]->GetResultType() == search::Result::RESULT_FEATURE)
  { // center viewport on clicked item
    m_pDrawWidget->ShowFeature(m_results[row]->GetFeatureRect());
  }
  else
  { // insert suggestion into the search bar
    string const suggestion = m_results[row]->GetSuggestionString();
    m_pEditor->setText(QString::fromUtf8(suggestion.c_str()));
  }
}

}
