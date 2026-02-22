#include "qt/user_search_panel/user_search_panel.hpp"
#include "qt/draw_widget.hpp"
#include "qt/user_search_panel/search_item.hpp"

#include "search/mode.hpp"
#include "search/search_params.hpp"

#include "map/bookmark_manager.hpp"
#include "map/framework.hpp"

#include "base/assert.hpp"

#include <qevent.h>
#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QStackedLayout>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>

namespace qt
{
UserSearchPanel::UserSearchPanel(Framework & framework, QWidget * parent)
  : QWidget(parent)
  , m_framework(framework)
  , m_timestamp(0)
{
  // TODO (@zagto): This "User" search panel implements features for depeloper mode aswell. Ideally
  // it will at some point become the only search panel. If that is not possible, these can be
  // removed.
  auto searchModeButtons = new QButtonGroup;
  m_devGroupBox = new QGroupBox;
  auto devLayout = new QVBoxLayout;
  QRadioButton * buttonEV = new QRadioButton(tr("Everywhere in List, Viewport in Map"));
  devLayout->addWidget(buttonEV);
  searchModeButtons->addButton(buttonEV, static_cast<int>(Mode::EverywhereAndViewport));
  QRadioButton * buttonE = new QRadioButton(tr("Everywhere"));
  devLayout->addWidget(buttonE);
  searchModeButtons->addButton(buttonE, static_cast<int>(Mode::Everywhere));
  QRadioButton * buttonV = new QRadioButton(tr("Viewport"));
  devLayout->addWidget(buttonV);
  searchModeButtons->addButton(buttonV, static_cast<int>(Mode::Viewport));
  m_devGroupBox->setLayout(devLayout);
  m_devGroupBox->setFlat(true);
  searchModeButtons->button(static_cast<int>(m_devSearchMode))->setChecked(true);
  connect(searchModeButtons, SIGNAL(idClicked(int)), this, SLOT(OnSearchModeChanged(int)));

  auto searchLocaleEdit = new QLineEdit;
  searchLocaleEdit->setPlaceholderText(tr("Search Locale"));
  connect(searchLocaleEdit, &QLineEdit::textChanged, this, &UserSearchPanel::OnDevSearchLocaleChanged);
  devLayout->addWidget(searchLocaleEdit);

  m_resultsList = new QListWidget;
  connect(m_resultsList, &QListWidget::clicked, this, &UserSearchPanel::OnListClicked);
  m_categoriesList = new QListWidget;
  m_historyList = new QListWidget;

  m_tabWidget = new QTabWidget;
  m_tabWidget->setDocumentMode(true);
  m_tabWidget->tabBar()->setExpanding(true);
  m_tabWidget->addTab(m_historyList, tr("History"));
  m_tabWidget->addTab(m_categoriesList, tr("Categories"));

  m_stackedLayout = new QStackedLayout;
  m_stackedLayout->addWidget(m_tabWidget);
  m_stackedLayout->addWidget(m_resultsList);

  auto mainLayout = new QVBoxLayout;
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addWidget(m_devGroupBox);
  mainLayout->addLayout(m_stackedLayout);
  setLayout(mainLayout);
}

void UserSearchPanel::RestartSearch()
{
  ClearResultsList();
  m_framework.GetSearchAPI().CancelAllSearches();
  RunSearch();
}

void UserSearchPanel::ClearResultsList()
{
  m_resultsList->clear();
  m_results.Clear();
}

void UserSearchPanel::OnEverywhereSearchResults(uint64_t timestamp, Mode mode, search::Results results)
{
  CHECK(m_threadChecker.CalledOnOriginalThread(), ());
  CHECK_LESS_OR_EQUAL(timestamp, m_timestamp, ());

  if (timestamp != m_timestamp)
    return;

  ClearResultsList();
  m_results = std::move(results);

  for (auto const & result : m_results)
  {
    // TODO (@zagto): use delegate
    auto listWidgetItem = new QListWidgetItem(m_resultsList);
    auto searchItem = new SearchItem(result, m_resultsList);
    m_resultsList->addItem(listWidgetItem);
    m_resultsList->setItemWidget(listWidgetItem, searchItem);
    listWidgetItem->setSizeHint(QSize(1, searchItem->heightForWidth(searchItem->width())));
  }

  // Viewport search updates the results in the view automatically. But for Everywhere-only mode, we
  // need to explicitly fill in the search results.
  if (mode == Mode::Everywhere)
    m_framework.FillSearchResultsMarks(true /* clear */, m_results);
}

bool UserSearchPanel::Try3dModeCmd(std::string const & str)
{
  bool const is3dModeOn = (str == "?3d");
  bool const is3dBuildingsOn = (str == "?b3d");
  bool const is3dModeOff = (str == "?2d");

  if (!is3dModeOn && !is3dBuildingsOn && !is3dModeOff)
    return false;

  m_framework.Save3dMode(is3dModeOn || is3dBuildingsOn, is3dBuildingsOn);
  m_framework.Allow3dMode(is3dModeOn || is3dBuildingsOn, is3dBuildingsOn);

  return true;
}

bool UserSearchPanel::TryTrafficSimplifiedColorsCmd(std::string const & str)
{
  bool const simplifiedMode = (str == "?tc:simp");
  bool const normalMode = (str == "?tc:norm");

  if (!simplifiedMode && !normalMode)
    return false;

  bool const isSimplified = simplifiedMode;
  m_framework.GetTrafficManager().SetSimplifiedColorScheme(isSimplified);
  m_framework.SaveTrafficSimplifiedColors(isSimplified);

  return true;
}

std::string UserSearchPanel::GetCurrentInputLocale()
{
  std::string resultLocale;
  if (m_developerMode && !m_devSearchLocale.empty())
    return m_devSearchLocale;

  QString loc = QGuiApplication::inputMethod()->locale().name();
  loc.replace('_', '-');
  auto res = loc.toStdString();
  if (CategoriesHolder::MapLocaleToInteger(res) < 0)
    res = "en";
  return res;
}

void UserSearchPanel::OnSearchQueryChanged(QString const & str)
{
  // Pass input query as-is without any normalization.
  // Otherwise № -> No, and it's unexpectable for the search index.
  m_searchQuery = str.toStdString();
  RunSearch();
}

void UserSearchPanel::OnDevSearchLocaleChanged(QString const & newValue)
{
  m_devSearchLocale = newValue.toStdString();
  RestartSearch();
}

void UserSearchPanel::RunSearch()
{
  if (Try3dModeCmd(m_searchQuery))
    return;
  if (TryTrafficSimplifiedColorsCmd(m_searchQuery))
    return;

  auto mode = Mode::EverywhereAndViewport;
  if (m_developerMode)
    mode = m_devSearchMode;

  bool const isCategory = false;  // TODO (@zagto): implement category UI
  auto const timestamp = ++m_timestamp;

  if (m_searchQuery.empty())
  {
    m_framework.GetSearchAPI().CancelAllSearches();
    ClearResultsList();
    m_stackedLayout->setCurrentIndex(static_cast<int>(LayoutIndex::HistoryAndCategories));
    return;
  }

  m_stackedLayout->setCurrentIndex(static_cast<int>(LayoutIndex::Results));
  // TODO (@zagto): busy indicator
  using namespace search;
  if (mode == Mode::EverywhereAndViewport || mode == Mode::Everywhere)
  {
    EverywhereSearchParams params{m_searchQuery,
                                  GetCurrentInputLocale(),
                                  {} /* timeout */,
                                  isCategory,
                                  // m_onResults
                                  [this, mode, timestamp](Results results, std::vector<ProductInfo> /* productInfo */)
    { OnEverywhereSearchResults(timestamp, mode, std::move(results)); }};

    m_framework.GetSearchAPI().SearchEverywhere(std::move(params));
  }
  if (mode == Mode::EverywhereAndViewport || mode == Mode::Viewport)
  {
    ViewportSearchParams::OnCompleted onCompleted = nullptr;
    if (mode == Mode::Viewport)
    {
      onCompleted = [this, mode, timestamp](Results results)
      { OnEverywhereSearchResults(timestamp, mode, std::move(results)); };
    }

    ViewportSearchParams params{m_searchQuery,
                                GetCurrentInputLocale(),
                                {} /* timeout */,
                                isCategory,
                                // m_onStarted
                                []() {},  // TODO (@zagto): busy indicator
                                onCompleted};
    m_framework.GetSearchAPI().SearchInViewport(std::move(params));
  }
}

void UserSearchPanel::OnSearchModeChanged(int mode)
{
  if (mode == static_cast<int>(m_devSearchMode))
    return;

  m_devSearchMode = static_cast<Mode>(mode);
  RestartSearch();
}

void UserSearchPanel::OnListClicked(QModelIndex const & index)
{
  int row = index.row();
  ASSERT_EQUAL(m_results.GetCount(), static_cast<size_t>(m_resultsList->count()), ());
  ASSERT_LESS(row, m_resultsList->count(), ());

  if (m_results[row].IsSuggest())
  {
    ClearResultsList();
    std::string const suggestion = m_results[row].GetSuggestionString();
    SuggestionAccepted(QString::fromUtf8(suggestion.c_str()));
  }
  else
  {
    // TODO (@zagto): this should not cancel the viewport search on desktop
    m_framework.ShowSearchResult(m_results[row]);
  }
}

void UserSearchPanel::resizeEvent(QResizeEvent * event)
{
  QWidget::resizeEvent(event);

  for (int row = 0; row < m_resultsList->count(); row++)
  {
    auto listWidgetItem = m_resultsList->item(row);
    auto searchItem = m_resultsList->itemWidget(listWidgetItem);
    listWidgetItem->setSizeHint(QSize(1, searchItem->heightForWidth(searchItem->width())));
  }

  WidthChanged(event->size().width());
}

void UserSearchPanel::hideEvent(QHideEvent *)
{
  m_framework.GetSearchAPI().CancelAllSearches();
}

QSize UserSearchPanel::sizeHint() const
{
  return QSize(kDefaultWidth, kDefaultWidth);
}

void UserSearchPanel::OnDeveloperModeChanged(bool enable)
{
  m_developerMode = enable;
  m_devGroupBox->setVisible(enable);
  RestartSearch();
}

}  // namespace qt
