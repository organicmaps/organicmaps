#pragma once

#include "search/mode.hpp"
#include "search/result.hpp"

#include "base/thread_checker.hpp"

#include <vector>

#include <QtGui/QPixmap>
#include <QtWidgets/QWidget>

class QTableWidget;
class QLineEdit;
class QPushButton;
class QButtonGroup;
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
  QButtonGroup * m_pSearchModeButtons;

  QPixmap m_busyIcon;

  std::vector<search::Result> m_results;

  search::Mode m_mode;
  uint64_t m_timestamp;

  ThreadChecker m_threadChecker;

  Q_OBJECT

public:
  SearchPanel(DrawWidget * drawWidget, QWidget * parent);

private:
  virtual void hideEvent(QHideEvent *);

  void ClearResults();

  void StartBusyIndicator();
  void StopBusyIndicator();

private slots:
  void OnSearchModeChanged(int mode);
  void OnSearchPanelItemClicked(int row, int column);
  void OnSearchTextChanged(QString const &);
  void OnEverywhereSearchResults(uint64_t timestamp, search::Results results);

  void OnAnimationTimer();
  void OnClearButton();

  bool Try3dModeCmd(QString const & str);
  bool TryTrafficSimplifiedColorsCmd(QString const & str);
};
}  // namespace qt
