#pragma once

#include "map/everywhere_search_params.hpp"

#include "search/result.hpp"

#include "base/thread_checker.hpp"

#include <cstdint>
#include <vector>

#include <QtGui/QPixmap>
#include <QtWidgets/QWidget>

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
