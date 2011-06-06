#include "search_panel.hpp"
#include "draw_widget.hpp"

#include "../std/bind.hpp"

#include <QtCore/QTimer>
#include <QtGui/QHeaderView>
#include <QtGui/QTableWidget>
#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>

namespace qt
{

SearchPanel::SearchPanel(DrawWidget * drawWidget, QWidget * parent)
  : QWidget(parent), m_pDrawWidget(drawWidget), m_queryId(0)
{
  m_pEditor = new QLineEdit(this);
  connect(m_pEditor, SIGNAL(textChanged(QString const &)), this, SLOT(OnSearchTextChanged(QString const &)));

  m_pTable = new QTableWidget(0, 2, this);
  m_pTable->setFocusPolicy(Qt::NoFocus);
  m_pTable->setAlternatingRowColors(true);
  m_pTable->setShowGrid(false);
  m_pTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_pTable->verticalHeader()->setVisible(false);
  m_pTable->horizontalHeader()->setVisible(false);
  m_pTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  connect(m_pTable, SIGNAL(cellClicked(int,int)), this, SLOT(OnSearchPanelItemClicked(int,int)));

  m_pClearButton = new QPushButton(this);
  connect(m_pClearButton, SIGNAL(pressed()), this, SLOT(OnClearButton()));
  m_pClearButton->setVisible(false);
  m_pClearButton->setFocusPolicy(Qt::NoFocus);
  m_pAnimationTimer = new QTimer(this);
  connect(m_pAnimationTimer, SIGNAL(timeout()), this, SLOT(OnAnimationTimer()));

  QHBoxLayout * horizontalLayout = new QHBoxLayout();
  horizontalLayout->addWidget(m_pEditor);
  horizontalLayout->addWidget(m_pClearButton);
  QVBoxLayout * verticalLayout = new QVBoxLayout();
  verticalLayout->addLayout(horizontalLayout);
  verticalLayout->addWidget(m_pTable);
  setLayout(verticalLayout);

  // for multithreading support
  CHECK(connect(this, SIGNAL(SearchResultSignal(search::Result *, int)),
                this, SLOT(OnSearchResult(search::Result *, int)), Qt::QueuedConnection), ());

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

void SearchPanel::SearchResultThreadFunc(search::Result const & result, int queryId)
{
  if (queryId == m_queryId)
    emit SearchResultSignal(new search::Result(result), queryId);
}

void SearchPanel::OnSearchResult(search::Result * result, int queryId)
{
  if (queryId != m_queryId)
    return;

  if (!result->GetString().empty())
  {
    int const rowCount = m_pTable->rowCount();
    m_pTable->setRowCount(rowCount + 1);
    QTableWidgetItem * item = new QTableWidgetItem(QString::fromUtf8(result->GetString().c_str()));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    m_pTable->setItem(rowCount, 0, item);
    m_results.push_back(result);
  }
  else
  { // last element
    delete result;
    // stop search busy indicator
    m_pAnimationTimer->stop();
    m_pClearButton->setIcon(QIcon(":/ui/x.png"));
  }
}

void SearchPanel::OnSearchTextChanged(QString const & str)
{
  // clear old results
  m_pTable->clear();
  m_pTable->setRowCount(0);
  ClearVector(m_results);
  ++m_queryId;

  QString const normalized = str.normalized(QString::NormalizationForm_KC);
  if (!normalized.isEmpty())
  {
    m_pDrawWidget->Search(normalized.toUtf8().constData(),
                        bind(&SearchPanel::SearchResultThreadFunc, this, _1, m_queryId));
    // show busy indicator
    if (!m_pAnimationTimer->isActive())
      m_pAnimationTimer->start(200);
    OnAnimationTimer();
    m_pClearButton->setFlat(true);
    m_pClearButton->setVisible(true);
  }
  else
  { // hide X button
    m_pClearButton->setVisible(false);
  }
}

void SearchPanel::OnSearchPanelItemClicked(int row, int)
{
  disconnect(m_pDrawWidget, SIGNAL(ViewportChanged()), this, SLOT(OnViewportChanged()));
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
  connect(m_pDrawWidget, SIGNAL(ViewportChanged()), this, SLOT(OnViewportChanged()));
}

void SearchPanel::showEvent(QShowEvent *)
{
  connect(m_pDrawWidget, SIGNAL(ViewportChanged()), this, SLOT(OnViewportChanged()));
}

void SearchPanel::hideEvent(QHideEvent *)
{
  disconnect(m_pDrawWidget, SIGNAL(ViewportChanged()), this, SLOT(OnViewportChanged()));
}

void SearchPanel::OnViewportChanged()
{
  QString const txt = m_pEditor->text();
  if (!txt.isEmpty())
    OnSearchTextChanged(txt);
}

void SearchPanel::OnAnimationTimer()
{
  static int angle = 0;
  QPixmap pixmap(":/ui/busy.png");
  QSize const oldSize = pixmap.size();
  QMatrix rm;
  angle += 15;
  if (angle >= 360)
    angle = 0;
  rm.rotate(angle);
  pixmap = pixmap.transformed(rm);
  m_pClearButton->setIcon(QIcon(pixmap));
}

void SearchPanel::OnClearButton()
{
  m_pEditor->setText("");
}

}
