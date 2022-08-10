#include "qt/search_panel.hpp"
#include "qt/draw_widget.hpp"

#include "search/search_params.hpp"

#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"
#include "map/user_mark_layer.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"

#include <functional>

#include <QtCore/QTimer>
#include <QtGui/QBitmap>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>

namespace qt
{
SearchPanel::SearchPanel(DrawWidget * drawWidget, QWidget * parent)
  : QWidget(parent)
  , m_pDrawWidget(drawWidget)
  , m_busyIcon(":/ui/busy.png")
  , m_mode(search::Mode::Everywhere)
  , m_timestamp(0)
{
  m_pEditor = new QLineEdit(this);
  connect(m_pEditor, &QLineEdit::textChanged, this, &SearchPanel::OnSearchTextChanged);

  m_pTable = new QTableWidget(0, 4 /*columns*/, this);
  m_pTable->setFocusPolicy(Qt::NoFocus);
  m_pTable->setAlternatingRowColors(true);
  m_pTable->setShowGrid(false);
  m_pTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_pTable->verticalHeader()->setVisible(false);
  m_pTable->horizontalHeader()->setVisible(false);
  m_pTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

  connect(m_pTable, &QTableWidget::cellClicked, this, &SearchPanel::OnSearchPanelItemClicked);

  m_pClearButton = new QPushButton(this);
  connect(m_pClearButton, &QAbstractButton::pressed, this, &SearchPanel::OnClearButton);
  m_pClearButton->setVisible(false);
  m_pClearButton->setFocusPolicy(Qt::NoFocus);
  m_pAnimationTimer = new QTimer(this);
  connect(m_pAnimationTimer, &QTimer::timeout, this, &SearchPanel::OnAnimationTimer);

  m_pSearchModeButtons = new QButtonGroup(this);
  QGroupBox * groupBox = new QGroupBox();
  QHBoxLayout * modeLayout = new QHBoxLayout();
  QRadioButton * buttonE = new QRadioButton("Everywhere");
  modeLayout->addWidget(buttonE);
  m_pSearchModeButtons->addButton(buttonE, static_cast<int>(search::Mode::Everywhere));
  QRadioButton * buttonV = new QRadioButton("Viewport");
  modeLayout->addWidget(buttonV);
  m_pSearchModeButtons->addButton(buttonV, static_cast<int>(search::Mode::Viewport));
  groupBox->setLayout(modeLayout);
  groupBox->setFlat(true);
  m_pSearchModeButtons->button(static_cast<int>(search::Mode::Everywhere))->setChecked(true);
  connect(m_pSearchModeButtons, SIGNAL(buttonClicked(int)), this, SLOT(OnSearchModeChanged(int)));

  QHBoxLayout * requestLayout = new QHBoxLayout();
  requestLayout->addWidget(m_pEditor);
  requestLayout->addWidget(m_pClearButton);
  QVBoxLayout * verticalLayout = new QVBoxLayout();
  verticalLayout->addWidget(groupBox);
  verticalLayout->addLayout(requestLayout);
  verticalLayout->addWidget(m_pTable);
  setLayout(verticalLayout);
}

namespace
{
QTableWidgetItem * CreateItem(QString const & s)
{
  QTableWidgetItem * item = new QTableWidgetItem(s);
  item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
  return item;
}
}  // namespace

void SearchPanel::ClearResults()
{
  m_pTable->clear();
  m_pTable->setRowCount(0);
  m_results.clear();
  m_pDrawWidget->GetFramework().GetBookmarkManager().GetEditSession().ClearGroup(UserMark::Type::SEARCH);
}

void SearchPanel::StartBusyIndicator()
{
  if (!m_pAnimationTimer->isActive())
    m_pAnimationTimer->start(200 /* milliseconds */);

  m_pClearButton->setFlat(true);
  m_pClearButton->setVisible(true);
}

void SearchPanel::StopBusyIndicator()
{
  m_pAnimationTimer->stop();
  m_pClearButton->setIcon(QIcon(":/ui/x.png"));
}

void SearchPanel::OnEverywhereSearchResults(uint64_t timestamp, search::Results results)
{
  CHECK(m_threadChecker.CalledOnOriginalThread(), ());
  CHECK_LESS_OR_EQUAL(timestamp, m_timestamp, ());

  if (timestamp != m_timestamp)
    return;

  CHECK_LESS_OR_EQUAL(m_results.size(), results.GetCount(), ());

  auto const sizeBeforeUpdate = m_results.size();

  for (size_t i = m_results.size(); i < results.GetCount(); ++i)
  {
    auto & res = results[i];
    QString const name = QString::fromStdString(res.GetString());
    QString strHigh;
    int pos = 0;
    for (size_t r = 0; r < res.GetHighlightRangesCount(); ++r)
    {
      std::pair<uint16_t, uint16_t> const & range = res.GetHighlightRange(r);
      strHigh.append(name.midRef(pos, range.first - pos));
      strHigh.append("<font color=\"green\">");
      strHigh.append(name.midRef(range.first, range.second));
      strHigh.append("</font>");

      pos = range.first + range.second;
    }
    strHigh.append(name.midRef(pos));

    int const rowCount = m_pTable->rowCount();
    m_pTable->insertRow(rowCount);
    m_pTable->setCellWidget(rowCount, 1, new QLabel(strHigh));
    m_pTable->setItem(rowCount, 2, CreateItem(QString::fromStdString(res.GetAddress())));

    if (res.GetResultType() == search::Result::Type::Feature)
    {
      std::string readableType = classif().GetReadableObjectName(res.GetFeatureType());
      m_pTable->setItem(rowCount, 0, CreateItem(QString::fromStdString(readableType)));
      m_pTable->setItem(rowCount, 3, CreateItem(m_pDrawWidget->GetDistance(res).c_str()));
    }

    m_results.push_back(std::move(res));
  }

  m_pDrawWidget->GetFramework().FillSearchResultsMarks(m_results.begin() + sizeBeforeUpdate,
                                                       m_results.end(), false /* clear */);

  if (results.IsEndMarker())
    StopBusyIndicator();
}

bool SearchPanel::Try3dModeCmd(QString const & str)
{
  bool const is3dModeOn = (str == "?3d");
  bool const is3dBuildingsOn = (str == "?b3d");
  bool const is3dModeOff = (str == "?2d");

  if (!is3dModeOn && !is3dBuildingsOn && !is3dModeOff)
    return false;

  m_pDrawWidget->GetFramework().Save3dMode(is3dModeOn || is3dBuildingsOn, is3dBuildingsOn);
  m_pDrawWidget->GetFramework().Allow3dMode(is3dModeOn || is3dBuildingsOn, is3dBuildingsOn);

  return true;
}

bool SearchPanel::TryTrafficSimplifiedColorsCmd(QString const & str)
{
  bool const simplifiedMode = (str == "?tc:simp");
  bool const normalMode = (str == "?tc:norm");

  if (!simplifiedMode && !normalMode)
    return false;

  bool const isSimplified = simplifiedMode;
  m_pDrawWidget->GetFramework().GetTrafficManager().SetSimplifiedColorScheme(isSimplified);
  m_pDrawWidget->GetFramework().SaveTrafficSimplifiedColors(isSimplified);

  return true;
}

void SearchPanel::OnSearchTextChanged(QString const & normalized)
{
  // Pass input query as-is without any normalization.
  // Otherwise № -> No, and it's unexpectable for the search index.
  //QString const normalized = str.normalized(QString::NormalizationForm_KC);

  if (Try3dModeCmd(normalized))
    return;
  if (TryTrafficSimplifiedColorsCmd(normalized))
    return;

  ClearResults();

  if (normalized.isEmpty())
  {
    m_pDrawWidget->GetFramework().GetSearchAPI().CancelAllSearches();

    // hide X button
    m_pClearButton->setVisible(false);
    return;
  }

  bool started = false;
  auto const timestamp = ++m_timestamp;

  using namespace search;
  switch (m_mode)
  {
  case Mode::Everywhere:
  {
    EverywhereSearchParams params{
      normalized.toUtf8().constData(), {} /* locale */, {} /* timeout */, false /* isCategory */,
      // m_onResults
      [this, timestamp](Results results, std::vector<ProductInfo> /* productInfo */)
      {
        OnEverywhereSearchResults(timestamp, std::move(results));
      }
    };

    started = m_pDrawWidget->GetFramework().GetSearchAPI().SearchEverywhere(std::move(params));
  }
  break;

  case Mode::Viewport:
  {
    ViewportSearchParams params{
      normalized.toUtf8().constData(), {} /* locale*/, {} /* timeout */, false /* isCategory */,
      // m_onStarted
      {},
      // m_onCompleted
      [this](search::Results results)
      {
        // |m_pTable| is not updated here because the OnResults callback is recreated within
        // SearchAPI when the viewport is changed. Thus a single call to SearchInViewport may
        // initiate an arbitrary amount of actual search requests with different viewports, and
        // clearing the table would require additional care (or, most likely, we would need a better
        // API). This is similar to the Android and iOS clients where we do not show the list of
        // results in the viewport search mode.
        m_pDrawWidget->GetFramework().FillSearchResultsMarks(true /* clear */, results);
        StopBusyIndicator();
      }
    };

    started = m_pDrawWidget->GetFramework().GetSearchAPI().SearchInViewport(std::move(params));
  }
  break;

  default:
    started = false;
  break;
  }

  if (started)
    StartBusyIndicator();
}

void SearchPanel::OnSearchModeChanged(int mode)
{
  auto const newMode = static_cast<search::Mode>(mode);
  switch (newMode)
  {
  case search::Mode::Everywhere:
  case search::Mode::Viewport:
    break;
  default:
    UNREACHABLE();
  }

  if (m_mode == newMode)
    return;

  m_mode = newMode;

  // Run this query in the new mode.
  auto const text = m_pEditor->text();
  m_pEditor->setText(QString());
  m_pEditor->setText(text);
}

void SearchPanel::OnSearchPanelItemClicked(int row, int)
{
  ASSERT_EQUAL(m_results.size(), static_cast<size_t>(m_pTable->rowCount()), ());

  if (m_results[row].IsSuggest())
  {
    // insert suggestion into the search bar
    std::string const suggestion = m_results[row].GetSuggestionString();
    m_pEditor->setText(QString::fromUtf8(suggestion.c_str()));
  }
  else
  {
    // center viewport on clicked item
    m_pDrawWidget->GetFramework().ShowSearchResult(m_results[row]);
  }
}

void SearchPanel::hideEvent(QHideEvent *)
{
  m_pDrawWidget->GetFramework().GetSearchAPI().CancelSearch(search::Mode::Everywhere);
}

void SearchPanel::OnAnimationTimer()
{
  static int angle = 0;

  QTransform transform;
  angle += 15;
  if (angle >= 360)
    angle = 0;
  transform.rotate(angle);

  m_pClearButton->setIcon(QIcon(m_busyIcon.transformed(transform)));
}

void SearchPanel::OnClearButton()
{
  m_pEditor->setText("");
}
}  // namespace qt
