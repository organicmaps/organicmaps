#include "sailfish/search_model.hpp"

#include "sailfish/app_settings.hpp"
#include "sailfish/framework_access.hpp"
#include "sailfish/helpers.hpp"

#include "map/everywhere_search_params.hpp"
#include "map/framework.hpp"
#include "map/search_api.hpp"
#include "map/viewport_search_params.hpp"

#include "search/displayed_categories.hpp"
#include "search/result.hpp"

#include "platform/distance.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <QPointer>
#include <QVariantMap>

#include <utility>

namespace sailfish
{
namespace
{
SearchModel::OpenState GetOpenState(search::Result const & result)
{
  switch (result.IsOpenNow())
  {
  case osm::Yes: return result.GetMinutesUntilClosed() < 60 ? SearchModel::ClosingSoon : SearchModel::Open;
  case osm::No: return result.GetMinutesUntilOpen() < 60 ? SearchModel::OpeningSoon : SearchModel::Closed;
  case osm::Unknown: return SearchModel::OpenUnknown;
  }
  return SearchModel::OpenUnknown;
}

// The highlight ranges index the code points (strings::UniString), not the UTF-16 text.
template <typename RangeFn>
QString Highlighted(std::string const & text, size_t rangesCount, RangeFn && range, QColor const & color)
{
  auto const codePoints = QString::fromStdString(text).toUcs4();
  auto const part = [&codePoints](int first, int length)
  { return QString::fromUcs4(codePoints.constData() + first, length).toHtmlEscaped(); };
  QString const open = QStringLiteral("<font color=\"%1\">").arg(color.name());
  QString styled;
  int pos = 0;
  for (size_t i = 0; i < rangesCount; ++i)
  {
    auto const & [first, length] = range(i);
    styled += part(pos, first - pos) + open + part(first, length) + QStringLiteral("</font>");
    pos = first + length;
  }
  return styled + part(pos, codePoints.size() - pos);
}
}  // namespace

SearchModel::SearchModel(QObject * parent)
  : QAbstractListModel(parent)
  , m_framework(GetFramework())
  , m_results(std::make_unique<search::Results>())
{
  m_runTimer.setSingleShot(true);
  m_runTimer.setInterval(200);
  connect(&m_runTimer, &QTimer::timeout, this, &SearchModel::Run);
}

SearchModel::~SearchModel()
{
  // Also clears the viewport search marks.
  m_framework.GetSearchAPI().CancelAllSearches();
}

int SearchModel::rowCount(QModelIndex const & parent) const
{
  return parent.isValid() ? 0 : static_cast<int>(m_results->GetCount());
}

QVariant SearchModel::data(QModelIndex const & index, int role) const
{
  if (!index.isValid() || index.row() >= rowCount())
    return {};

  auto const & result = (*m_results)[static_cast<size_t>(index.row())];
  bool const isFeature = result.GetResultType() == search::Result::Type::Feature;
  switch (role)
  {
  case NameRole:
    // Unnamed places are titled by their type.
    if (result.GetString().empty() && isFeature)
      return QString::fromStdString(result.GetLocalizedFeatureType()).toHtmlEscaped();
    return Highlighted(result.GetString(), result.GetHighlightRangesCount(),
                       [&](size_t i) { return result.GetHighlightRange(i); }, m_highlightColor);
  case DescriptionRole:
    return isFeature ? QString::fromStdString(result.GetFeatureDescription(result.GetLocalizedFeatureType()))
                     : QString();
  case AddressRole:
    return Highlighted(result.GetAddress(), result.GetDescHighlightRangesCount(),
                       [&](size_t i) { return result.GetDescHighlightRange(i); }, m_highlightColor);
  case DistanceRole:
  {
    auto const position = m_framework.GetCurrentPosition();
    if (!position || !result.HasPoint() || result.IsSuggest())
      return QString();
    return QString::fromStdString(
        platform::Distance::CreateFormatted(mercator::DistanceOnEarth(*position, result.GetFeatureCenter()))
            .ToString());
  }
  case OpenStatusRole:
    switch (GetOpenState(result))
    {
    case Open: return Localized("editor_time_open");
    case ClosingSoon: return Localized("closes_in", {FormatDuration(result.GetMinutesUntilClosed() * 60L)});
    case OpeningSoon: return Localized("opens_in", {FormatDuration(result.GetMinutesUntilOpen() * 60L)});
    case Closed: return Localized("closed");
    case OpenUnknown: return QString();
    }
    return QString();
  case OpenStateRole: return GetOpenState(result);
  case SuggestRole: return result.IsSuggest();
  default: return {};
  }
}

QHash<int, QByteArray> SearchModel::roleNames() const
{
  return {{NameRole, "name"},         {DescriptionRole, "description"}, {AddressRole, "address"},
          {DistanceRole, "distance"}, {OpenStatusRole, "openStatus"},   {OpenStateRole, "openState"},
          {SuggestRole, "suggest"}};
}

void SearchModel::setQuery(QString const & query)
{
  if (query == m_query)
    return;
  m_query = query;
  m_linkLocale.clear();
  emit queryChanged();
  if (m_query.isEmpty())
    Run();
  else
    m_runTimer.start();
}

QVariantList SearchModel::categories() const
{
  QVariantList categories;
  auto const & displayed = m_framework.GetDisplayedCategories();
  auto const locale = GetInputLocale();
  for (auto const & key : displayed.GetKeys())
  {
    // The first synonym in the search language is the category name, with English as a fallback.
    std::string name, english;
    displayed.ForEachSynonym(key, [&](std::string const & synonym, std::string const & synonymLocale)
    {
      if (name.empty() && synonymLocale == locale)
        name = synonym;
      if (english.empty() && synonymLocale == "en")
        english = synonym;
    });
    QVariantMap category;
    category["key"] = QString::fromStdString(key);
    category["name"] = QString::fromStdString(name.empty() ? english : name);
    categories.append(category);
  }
  return categories;
}

void SearchModel::searchCategory(QString const & name, bool addToHistory)
{
  // The trailing space tells the search that the category name is complete.
  m_categoryQuery = name + ' ';
  setQuery(m_categoryQuery);
  if (addToHistory)
    SaveToHistory(name);
}

void SearchModel::searchFromLink(QString const & query, QString const & locale)
{
  setQuery(query);
  m_linkLocale = locale.toStdString();
}

bool SearchModel::activate(int row)
{
  ASSERT(row >= 0 && row < rowCount(), (row));
  auto const & result = (*m_results)[static_cast<size_t>(row)];
  if (result.IsSuggest())
  {
    setQuery(QString::fromStdString(result.GetSuggestionString()));
    return false;
  }
  SaveToHistory(m_query);
  m_framework.SelectSearchResult(result, true /* animation */);
  return true;
}

void SearchModel::showOnMap()
{
  ASSERT_GREATER(m_results->GetCount(), 0, ());
  SaveToHistory(m_query);
  m_framework.UpdateViewport(*m_results);
}

QStringList SearchModel::history() const
{
  QStringList history;
  for (auto const & request : m_framework.GetSearchAPI().GetLastSearchQueries())
    history.append(QString::fromStdString(request.second));
  return history;
}

void SearchModel::clearHistory()
{
  m_framework.GetSearchAPI().ClearSearchHistory();
  emit historyChanged();
}

void SearchModel::SaveToHistory(QString const & query)
{
  QString const trimmed = query.trimmed();
  if (trimmed.isEmpty() || !AppSettings::IsSearchHistoryEnabled())
    return;
  m_framework.GetSearchAPI().SaveSearchQuery({GetInputLocale(), trimmed.toStdString()});
  emit historyChanged();
}

void SearchModel::Run()
{
  m_runTimer.stop();
  // The old results stay until the new ones arrive.
  auto const timestamp = ++m_timestamp;
  auto & api = m_framework.GetSearchAPI();
  if (m_query.isEmpty())
  {
    beginResetModel();
    m_results->Clear();
    endResetModel();
    api.CancelAllSearches();
    SetSearching(false);
    return;
  }

  bool const isCategory = m_query == m_categoryQuery;
  // The keyboard may have changed since the last search.
  auto const locale = m_linkLocale.empty() ? GetInputLocale() : m_linkLocale;
  // Draws the result marks; SearchAPI repeats it whenever the map moves, until the query is cleared.
  api.SearchInViewport(
      {m_query.toStdString(), locale, {} /* timeout */, isCategory, {} /* onStarted */, {} /* onCompleted */});

  // Results arrive on the GUI thread and may outlive the page that owns the model.
  QPointer<SearchModel> self(this);
  search::EverywhereSearchParams params{m_query.toStdString(),
                                        locale,
                                        {} /* timeout */,
                                        isCategory,
                                        [self, timestamp](search::Results results)
  {
    if (self)
      self->OnResults(timestamp, std::move(results));
  }};
  SetSearching(api.SearchEverywhere(std::move(params)));
}

void SearchModel::OnResults(uint64_t timestamp, search::Results && results)
{
  if (timestamp != m_timestamp)
    return;

  beginResetModel();
  *m_results = std::move(results);
  endResetModel();
  if (m_results->IsEndMarker())
    SetSearching(false);
}

void SearchModel::SetSearching(bool searching)
{
  if (searching == m_searching)
    return;
  m_searching = searching;
  emit searchingChanged();
}
}  // namespace sailfish
