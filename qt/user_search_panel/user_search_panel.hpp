#pragma once

#include "search/result.hpp"

#include "base/thread_checker.hpp"

#include <QtWidgets/QWidget>

class QCheckBox;
class QGroupBox;
class QListWidget;
class QModelIndex;
class QStackedLayout;
class QTabWidget;

class Framework;

namespace qt
{
class DrawWidget;

class UserSearchPanel : public QWidget
{
  Q_OBJECT

public:
  static constexpr int kDefaultWidth = 350;

  UserSearchPanel(Framework & framework, QWidget * parent = nullptr);

  void OnDeveloperModeChanged(bool enable);

signals:
  void WidthChanged(int width);
  void SuggestionAccepted(QString const & searchQuery);

public slots:
  void OnSearchQueryChanged(QString const & str);

protected:
  void resizeEvent(QResizeEvent * event) override;
  void hideEvent(QHideEvent *) override;

  QSize sizeHint() const override;

private:
  enum class LayoutIndex
  {
    HistoryAndCategories,
    Results,
  };
  enum class Mode
  {
    EverywhereAndViewport,
    Everywhere,
    Viewport,
  };

  void RunSearch();
  void RestartSearch();
  void ClearResultsList();
  void OnEverywhereSearchResults(uint64_t timestamp, Mode mode, search::Results results);

  bool Try3dModeCmd(std::string const & str);
  bool TryTrafficSimplifiedColorsCmd(std::string const & str);

  std::string GetCurrentInputLocale();

private slots:
  void OnListClicked(QModelIndex const & index);
  void OnSearchModeChanged(int mode);
  void OnDevSearchLocaleChanged(QString const & newValue);

private:
  Framework & m_framework;

  QGroupBox * m_devGroupBox = nullptr;

  QListWidget * m_resultsList = nullptr;
  QListWidget * m_categoriesList = nullptr;
  QListWidget * m_historyList = nullptr;
  QTabWidget * m_tabWidget = nullptr;
  QStackedLayout * m_stackedLayout = nullptr;

  std::string m_searchQuery;
  bool m_developerMode = true;
  Mode m_devSearchMode = Mode::EverywhereAndViewport;
  std::string m_devSearchLocale;

  search::Results m_results;

  uint64_t m_timestamp = 0;

  ThreadChecker m_threadChecker;
};
}  // namespace qt
