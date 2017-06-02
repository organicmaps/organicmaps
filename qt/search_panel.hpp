#pragma once

#include "search/result.hpp"
#include "search/everywhere_search_params.hpp"

#include "base/thread_checker.hpp"

#include "std/cstdint.hpp"
#include "std/vector.hpp"

#include <QtGui/QPixmap>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QWidget>
#else
  #include <QtWidgets/QWidget>
#endif

class QTableWidget;
class QLineEdit;
class QPushButton;
class QTimer;

namespace qt
{
class DrawWidget;

class SearchPanel : public QWidget
{
  DrawWidget * m_pDrawWidget;
  QTableWidget * m_pTable;
  QLineEdit * m_pEditor;
  QPushButton * m_pClearButton;
  QTimer * m_pAnimationTimer;

  QPixmap m_busyIcon;

  vector<search::Result> m_results;

  search::EverywhereSearchParams m_params;
  uint64_t m_timestamp;

  ThreadChecker m_threadChecker;

  Q_OBJECT

public:
  SearchPanel(DrawWidget * drawWidget, QWidget * parent);

private:
  virtual void hideEvent(QHideEvent *);

  void ClearResults();

private slots:
  void OnSearchPanelItemClicked(int row, int column);
  void OnSearchTextChanged(QString const &);
  void OnSearchResults(uint64_t timestamp, search::Results const & results);

  void OnAnimationTimer();
  void OnClearButton();

  bool TryChangeRouterCmd(QString const & str);
  bool Try3dModeCmd(QString const & str);
  bool TryMigrate(QString const & str);
  bool TryDisplacementModeCmd(QString const & str);
  bool TryTrafficSimplifiedColorsCmd(QString const & str);
};
}  // namespace qt
