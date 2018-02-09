#include "qt/search_panel.hpp"
#include "qt/draw_widget.hpp"

#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"
#include "map/user_mark_layer.hpp"

#include "drape/constants.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/platform.hpp"

#include "base/assert.hpp"

#include <functional>

#include <QtCore/QTimer>
#include <QtGui/QBitmap>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>

namespace qt
{
SearchPanel::SearchPanel(DrawWidget * drawWidget, QWidget * parent)
  : QWidget(parent)
  , m_pDrawWidget(drawWidget)
  , m_busyIcon(":/ui/busy.png")
  , m_timestamp(0)
{
  m_pEditor = new QLineEdit(this);
  connect(m_pEditor, SIGNAL(textChanged(QString const &)),
          this, SLOT(OnSearchTextChanged(QString const &)));

  m_pTable = new QTableWidget(0, 4 /*columns*/, this);
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
}

void SearchPanel::OnSearchResults(uint64_t timestamp, search::Results const & results)
{
  CHECK(m_threadChecker.CalledOnOriginalThread(), ());
  CHECK_LESS_OR_EQUAL(timestamp, m_timestamp, ());

  if (timestamp != m_timestamp)
    return;

  CHECK_LESS_OR_EQUAL(m_results.size(), results.GetCount(), ());

  for (size_t i = m_results.size(); i < results.GetCount(); ++i)
  {
    auto const & res = results[i];
    QString const name = QString::fromStdString(res.GetString());
    QString strHigh;
    int pos = 0;
    for (size_t r = 0; r < res.GetHighlightRangesCount(); ++r)
    {
      pair<uint16_t, uint16_t> const & range = res.GetHighlightRange(r);
      strHigh.append(name.mid(pos, range.first - pos));
      strHigh.append("<font color=\"green\">");
      strHigh.append(name.mid(range.first, range.second));
      strHigh.append("</font>");

      pos = range.first + range.second;
    }
    strHigh.append(name.mid(pos));

    int const rowCount = m_pTable->rowCount();
    m_pTable->insertRow(rowCount);
    m_pTable->setCellWidget(rowCount, 1, new QLabel(strHigh));
    m_pTable->setItem(rowCount, 2, CreateItem(QString::fromStdString(res.GetAddress())));

    if (res.GetResultType() == search::Result::Type::Feature)
    {
      m_pTable->setItem(rowCount, 0, CreateItem(QString::fromStdString(res.GetFeatureTypeName())));
      m_pTable->setItem(rowCount, 3, CreateItem(m_pDrawWidget->GetDistance(res).c_str()));
    }

    m_results.push_back(res);
  }

  if (results.IsEndMarker())
  {
    // stop search busy indicator
    m_pAnimationTimer->stop();
    m_pClearButton->setIcon(QIcon(":/ui/x.png"));
  }
}

// TODO: This code only for demonstration purposes and will be removed soon
bool SearchPanel::TryChangeRouterCmd(QString const & str)
{
  routing::RouterType routerType;
  if (str == "?pedestrian")
    routerType = routing::RouterType::Pedestrian;
  else if (str == "?vehicle")
    routerType = routing::RouterType::Vehicle;
  else if (str == "?bicycle")
    routerType = routing::RouterType::Bicycle;
  else if (str == "?transit")
    routerType = routing::RouterType::Transit;
  else
    return false;

  m_pEditor->setText("");
  parentWidget()->hide();
  m_pDrawWidget->SetRouter(routerType);
  return true;
}

// TODO: This code only for demonstration purposes and will be removed soon
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

bool SearchPanel::TryMigrate(QString const & str)
{
  bool const isMigrate = (str == "?migrate");

  if (!isMigrate)
    return false;

  m_pEditor->setText("");
  parentWidget()->hide();

  auto const stateChanged = [&](storage::TCountryId const & id)
  {
    storage::Status const nextStatus = m_pDrawWidget->GetFramework().GetStorage().GetPrefetchStorage()->CountryStatusEx(id);
    LOG_SHORT(LINFO, (id, "status :", nextStatus));
    if (nextStatus == storage::Status::EOnDisk)
    {
      LOG_SHORT(LINFO, ("Prefetch done. Ready to migrate."));
      m_pDrawWidget->GetFramework().Migrate();
    }
  };

  auto const progressChanged = [](storage::TCountryId const & id, storage::MapFilesDownloader::TProgress const & sz)
  {
    LOG(LINFO, (id, "downloading progress:", sz));
  };

  ms::LatLon curPos(55.7, 37.7);

  m_pDrawWidget->GetFramework().PreMigrate(curPos, stateChanged, progressChanged);
  return true;

}

bool SearchPanel::TryDisplacementModeCmd(QString const & str)
{
  bool const isDefaultDisplacementMode = (str == "?dm:default");
  bool const isHotelDisplacementMode = (str == "?dm:hotel");

  if (!isDefaultDisplacementMode && !isHotelDisplacementMode)
    return false;

  if (isDefaultDisplacementMode)
  {
    m_pDrawWidget->GetFramework().SetDisplacementMode(DisplacementModeManager::SLOT_DEBUG,
                                                      false /* show */);
  }
  else if (isHotelDisplacementMode)
  {
    m_pDrawWidget->GetFramework().SetDisplacementMode(DisplacementModeManager::SLOT_DEBUG,
                                                      true /* show */);
  }

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

void SearchPanel::OnSearchTextChanged(QString const & str)
{
  QString const normalized = str.normalized(QString::NormalizationForm_KC);

  // TODO: This code only for demonstration purposes and will be removed soon
  if (TryChangeRouterCmd(normalized))
    return;
  if (Try3dModeCmd(normalized))
    return;
  if (TryMigrate(normalized))
    return;
  if (TryDisplacementModeCmd(normalized))
    return;
  if (TryTrafficSimplifiedColorsCmd(normalized))
    return;

  ClearResults();

  // search even with empty query
  if (!normalized.isEmpty())
  {
    m_params.m_query = normalized.toUtf8().constData();
    auto const timestamp = ++m_timestamp;
    m_params.m_onResults = [this, timestamp](search::Results const & results,
                                             std::vector<search::ProductInfo> const & productInfo) {
      GetPlatform().RunTask(Platform::Thread::Gui, bind(&SearchPanel::OnSearchResults, this,
                                                        timestamp, results));
    };

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
    m_pDrawWidget->GetFramework().CancelSearch(search::Mode::Everywhere);

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
  m_pDrawWidget->GetFramework().CancelSearch(search::Mode::Everywhere);
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
}  // namespace qt
