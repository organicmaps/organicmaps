#include "search_panel.hpp"
#include "draw_widget.hpp"

#include "../map/measurement_utils.hpp"

#include "../std/bind.hpp"

#include <QtCore/QTimer>

#include <QtGui/QBitmap>

#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>


namespace qt
{

SearchPanel::SearchPanel(DrawWidget * drawWidget, QWidget * parent)
  : QWidget(parent), m_pDrawWidget(drawWidget), m_busyIcon(":/ui/busy.png")
{
  m_pEditor = new QLineEdit(this);
  connect(m_pEditor, SIGNAL(textChanged(QString const &)),
          this, SLOT(OnSearchTextChanged(QString const &)));

  m_pTable = new QTableWidget(0, 5, this);
  m_pTable->setFocusPolicy(Qt::NoFocus);
  m_pTable->setAlternatingRowColors(true);
  m_pTable->setShowGrid(false);
  m_pTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_pTable->verticalHeader()->setVisible(false);
  m_pTable->horizontalHeader()->setVisible(false);
  m_pTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  connect(m_pTable, SIGNAL(cellClicked(int, int)), this, SLOT(OnSearchPanelItemClicked(int,int)));

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
  CHECK(connect(this, SIGNAL(SearchResultSignal(ResultsT *)),
                this, SLOT(OnSearchResult(ResultsT *)), Qt::QueuedConnection), ());

  /// @todo THis is not working on Qt5. QLineEdit doesn't get any input.
  //setFocusPolicy(Qt::StrongFocus);
  //setFocusProxy(m_pEditor);

  m_params.m_callback = bind(&SearchPanel::SearchResultThreadFunc, this, _1);
}

void SearchPanel::SearchResultThreadFunc(ResultsT const & result)
{
  emit SearchResultSignal(new ResultsT(result));
}

namespace
{
  QTableWidgetItem * create_item(QString const & s)
  {
    QTableWidgetItem * item = new QTableWidgetItem(s);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    return item;
  }

  /*
  QIcon draw_direction(double a)
  {
    int const dim = 64;

    QPixmap pm(dim, dim);

    QBitmap mask(dim, dim);
    mask.clear();
    pm.setMask(mask);

    QPainter painter(&pm);
    painter.setBackgroundMode(Qt::TransparentMode);

    QMatrix m;
    m.translate(dim/2, dim/2);
    m.rotate(-a / math::pi * 180.0);
    m.translate(-dim/2, -dim/2);

    typedef QPointF P;
    QPolygonF poly(5);
    poly[0] = P(dim/3, dim/2);
    poly[1] = P(0, dim/2 - dim/3);
    poly[2] = P(dim, dim/2);
    poly[3] = P(0, dim/2 + dim/3);
    poly[4] = P(dim/3, dim/2);

    painter.setBrush(Qt::black);
    painter.drawPolygon(m.map(poly));

    return pm;
  }
  */
}

void SearchPanel::OnSearchResult(ResultsT * res)
{
  scoped_ptr<ResultsT> guard(res);

  if (res->IsEndMarker())
  {
    if (res->IsEndedNormal())
    {
      // stop search busy indicator
      m_pAnimationTimer->stop();
      m_pClearButton->setIcon(QIcon(":/ui/x.png"));
    }
  }
  else
  {
    // clear old results
    m_pTable->clear();
    m_pTable->setRowCount(0);
    m_results.clear();

    Framework & frm = m_pDrawWidget->GetFramework();
    frm.AdditionalPoiLayerClear();

    for (ResultsT::IterT i = res->Begin(); i != res->End(); ++i)
    {
      ResultT const & e = *i;

      int const rowCount = m_pTable->rowCount();
      m_pTable->insertRow(rowCount);
      m_pTable->setItem(rowCount, 1, create_item(QString::fromUtf8(e.GetString())));
      m_pTable->setItem(rowCount, 2, create_item(QString::fromUtf8(e.GetRegionString())));

      if (e.GetResultType() == ResultT::RESULT_FEATURE)
      {
        // For debug purposes: add bookmarks for search results
        Bookmark bm(e.GetFeatureCenter(), e.GetString(), "placemark-red");
        frm.AdditionalPoiLayerAddPoi(bm);

        m_pTable->setItem(rowCount, 0, create_item(QString::fromUtf8(e.GetFeatureType())));

        m_pTable->setItem(rowCount, 3, create_item(m_pDrawWidget->GetDistance(e).c_str()));
      }

      m_results.push_back(e);
    }
  }
}

void SearchPanel::OnSearchTextChanged(QString const & str)
{
  QString const normalized = str.normalized(QString::NormalizationForm_KC);

  // search even with empty query
  //if (!normalized.isEmpty())
  {
    m_params.m_query = normalized.toUtf8().constData();
    if (m_pDrawWidget->Search(m_params))
    {
      // show busy indicator
      if (!m_pAnimationTimer->isActive())
        m_pAnimationTimer->start(200);

      m_pClearButton->setFlat(true);
      m_pClearButton->setVisible(true);
    }
  }
  //else
  //{
  //  // hide X button
  //  m_pClearButton->setVisible(false);
  //}
}

void SearchPanel::OnSearchPanelItemClicked(int row, int)
{
  disconnect(m_pDrawWidget, SIGNAL(ViewportChanged()), this, SLOT(OnViewportChanged()));
  ASSERT_EQUAL(m_results.size(), static_cast<size_t>(m_pTable->rowCount()), ());
  if (m_results[row].GetResultType() == ResultT::RESULT_FEATURE)
  {
    // center viewport on clicked item
    m_pDrawWidget->ShowSearchResult(m_results[row]);
  }
  else
  {
    // insert suggestion into the search bar
    string const suggestion = m_results[row].GetSuggestionString();
    m_pEditor->setText(QString::fromUtf8(suggestion.c_str()));
  }
  connect(m_pDrawWidget, SIGNAL(ViewportChanged()), this, SLOT(OnViewportChanged()));
}

void SearchPanel::showEvent(QShowEvent *)
{
  connect(m_pDrawWidget, SIGNAL(ViewportChanged()), this, SLOT(OnViewportChanged()));
  OnSearchTextChanged(QString());
}

void SearchPanel::hideEvent(QHideEvent *)
{
  m_pDrawWidget->GetFramework().AdditionalPoiLayerClear();

  disconnect(m_pDrawWidget, SIGNAL(ViewportChanged()), this, SLOT(OnViewportChanged()));

  m_pDrawWidget->CloseSearch();
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

  QMatrix rm;
  angle += 15;
  if (angle >= 360)
    angle = 0;
  rm.rotate(angle);

  m_pClearButton->setIcon(QIcon(m_busyIcon.transformed(rm)));
}

void SearchPanel::OnClearButton()
{
  m_pEditor->setText("");
}

}
