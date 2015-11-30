#include "qt/search_panel.hpp"
#include "qt/draw_widget.hpp"

#include "map/bookmark_manager.hpp"
#include "map/user_mark_container.hpp"

#include "platform/measurement_utils.hpp"

#include "std/bind.hpp"

#include <QtCore/QTimer>

#include <QtGui/QBitmap>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QHeaderView>
  #include <QtGui/QTableWidget>
  #include <QtGui/QLineEdit>
  #include <QtGui/QVBoxLayout>
  #include <QtGui/QHBoxLayout>
  #include <QtGui/QPushButton>
  #include <QtGui/QLabel>
#else
  #include <QtWidgets/QHeaderView>
  #include <QtWidgets/QTableWidget>
  #include <QtWidgets/QLineEdit>
  #include <QtWidgets/QVBoxLayout>
  #include <QtWidgets/QHBoxLayout>
  #include <QtWidgets/QPushButton>
  #include <QtWidgets/QLabel>
#endif

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
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  m_pTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#else
  m_pTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif

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
}

void SearchPanel::ClearResults()
{
  m_pTable->clear();
  m_pTable->setRowCount(0);
  m_results.clear();
}

void SearchPanel::OnSearchResult(ResultsT * res)
{
  unique_ptr<ResultsT> const guard(res);

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
    ClearResults();

    for (ResultsT::IterT i = res->Begin(); i != res->End(); ++i)
    {
      ResultT const & e = *i;

      QString s = QString::fromUtf8(e.GetString());
      QString strHigh;
      int pos = 0;
      for (size_t r = 0; r < e.GetHighlightRangesCount(); ++r)
      {
        pair<uint16_t, uint16_t> const & range = e.GetHighlightRange(r);
        strHigh.append(s.mid(pos, range.first - pos));
        strHigh.append("<font color=\"green\">");
        strHigh.append(s.mid(range.first, range.second));
        strHigh.append("</font>");

        pos = range.first + range.second;
      }
      strHigh.append(s.mid(pos));

      int const rowCount = m_pTable->rowCount();
      m_pTable->insertRow(rowCount);
      m_pTable->setCellWidget(rowCount, 1, new QLabel(strHigh));
      m_pTable->setItem(rowCount, 2, create_item(QString::fromUtf8(e.GetRegionString())));

      if (e.GetResultType() == ResultT::RESULT_FEATURE)
      {
        m_pTable->setItem(rowCount, 0, create_item(QString::fromUtf8(e.GetFeatureType())));
        m_pTable->setItem(rowCount, 3, create_item(m_pDrawWidget->GetDistance(e).c_str()));
      }

      m_results.push_back(e);
    }
  }
}

// TODO: This code only for demonstration purposes and will be removed soon
bool SearchPanel::TryChangeMapStyleCmd(QString const & str)
{
  // Hook for shell command on change map style
  bool const isDark = (str == "mapstyle:dark") || (str == "?dark");
  bool const isLight = isDark ? false : (str == "mapstyle:light") || (str == "?light");
  bool const isOld = isDark || isLight ? false : (str == "?oldstyle");

  if (!isDark && !isLight && !isOld)
    return false;

  // close Search panel
  m_pEditor->setText("");
  parentWidget()->hide();

  // change color scheme for the Map activity
  MapStyle const mapStyle = isOld ? MapStyleLight : (isDark ? MapStyleDark : MapStyleClear);
  m_pDrawWidget->SetMapStyle(mapStyle);

  return true;
}

// TODO: This code only for demonstration purposes and will be removed soon
bool SearchPanel::TryChangeRouterCmd(QString const & str)
{
  bool const isPedestrian = (str == "?pedestrian");
  bool const isVehicle = isPedestrian ? false : (str == "?vehicle");

  if (!isPedestrian && !isVehicle)
    return false;

  m_pEditor->setText("");
  parentWidget()->hide();

  routing::RouterType const routerType = isPedestrian ? routing::RouterType::Pedestrian : routing::RouterType::Vehicle;
  m_pDrawWidget->SetRouter(routerType);

  return true;
}


void SearchPanel::OnSearchTextChanged(QString const & str)
{
  QString const normalized = str.normalized(QString::NormalizationForm_KC);

  // TODO: This code only for demonstration purposes and will be removed soon
  if (TryChangeMapStyleCmd(normalized))
    return;
  if (TryChangeRouterCmd(normalized))
    return;

  // search even with empty query
  if (!normalized.isEmpty())
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
  else
  {
    ClearResults();

    m_pDrawWidget->GetFramework().CancelInteractiveSearch();

    // hide X button
    m_pClearButton->setVisible(false);
  }
}

void SearchPanel::OnSearchPanelItemClicked(int row, int)
{
  ASSERT_EQUAL(m_results.size(), static_cast<size_t>(m_pTable->rowCount()), ());

  if (m_results[row].IsSuggest())
  {
    // insert suggestion into the search bar
    string const suggestion = m_results[row].GetSuggestionString();
    m_pEditor->setText(QString::fromUtf8(suggestion.c_str()));
  }
  else
  {
    // center viewport on clicked item
    m_pDrawWidget->ShowSearchResult(m_results[row]);
  }
}

void SearchPanel::hideEvent(QHideEvent *)
{
  m_pDrawWidget->GetFramework().CancelInteractiveSearch();
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
