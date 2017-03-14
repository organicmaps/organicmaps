#pragma once

#include "search/result.hpp"
#include "search/everywhere_search_params.hpp"

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

  /// Stores current search results
  typedef search::Results ResultsT;
  typedef search::Result ResultT;
  vector<ResultT> m_results;

  search::EverywhereSearchParams m_params;

  Q_OBJECT

public:
  SearchPanel(DrawWidget * drawWidget, QWidget * parent);

private:
  virtual void hideEvent(QHideEvent *);

  void SearchResultThreadFunc(ResultsT const & result);
  void ClearResults();

signals:
  void SearchResultSignal(ResultsT * result);

private slots:
  void OnSearchPanelItemClicked(int row, int column);
  void OnSearchTextChanged(QString const &);

  /// Called via signal to support multithreading
  void OnSearchResult(ResultsT * results);

  void OnAnimationTimer();
  void OnClearButton();

  bool TryChangeRouterCmd(QString const & str);
  bool Try3dModeCmd(QString const & str);
  bool TryMigrate(QString const & str);
  bool TryDisplacementModeCmd(QString const & str);
  bool TryTrafficSimplifiedColorsCmd(QString const & str);
};
}  // namespace qt
