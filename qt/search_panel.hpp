#pragma once

#include "search/mode.hpp"
#include "search/result.hpp"

#include "base/thread_checker.hpp"

#include <vector>

#include <QtGui/QIcon>
#include <QtGui/QPixmap>
#include <QtWidgets/QWidget>

class QCheckBox;
class QLineEdit;
class QPushButton;
class QTableWidget;
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
  QCheckBox * m_isCategory;

  QIcon m_clearIcon;
  QPixmap m_busyIcon;

  search::Results m_results;

  search::Mode m_mode;
  uint64_t m_timestamp;

  ThreadChecker m_threadChecker;

  Q_OBJECT

public:
  SearchPanel(DrawWidget * drawWidget, QWidget * parent);

  static std::string GetCurrentInputLocale();

private:
  virtual void hideEvent(QHideEvent *);

  void RunSearch();
  void ClearTable();
  void ClearResults();

  void StartBusyIndicator();
  void StopBusyIndicator();

private slots:
  void OnSearchModeChanged(int mode);
  void OnSearchPanelItemClicked(int row, int column);
  void OnSearchTextChanged(QString const & str);
  void OnEverywhereSearchResults(uint64_t timestamp, search::Results results);

  void OnAnimationTimer();
  void OnClearButton();

  bool Try3dModeCmd(std::string const & str);
  bool TryTrafficSimplifiedColorsCmd(std::string const & str);
};
}  // namespace qt
