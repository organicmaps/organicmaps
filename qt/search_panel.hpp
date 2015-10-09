#pragma once

#include "search/result.hpp"
#include "search/params.hpp"

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

  search::SearchParams m_params;

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
  void OnSearchResult(ResultsT * result);

  void OnAnimationTimer();
  void OnClearButton();

  bool TryChangeMapStyleCmd(QString const & str);
  bool TryChangeRouterCmd(QString const & str);
  bool Try3dModeCmd(QString const & str);
};

}
