#pragma once

#include "../search/result.hpp"
#include "../search/params.hpp"

#include "../std/vector.hpp"

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

  /// Stores current search results
  typedef search::Results ResultsT;
  typedef search::Result ResultT;
  vector<ResultT> m_results;

  search::SearchParams m_params;

  Q_OBJECT

public:
  SearchPanel(DrawWidget * drawWidget, QWidget * parent);

private:
  virtual void showEvent(QShowEvent *);
  virtual void hideEvent(QHideEvent *);

  void SearchResultThreadFunc(ResultsT const & result);

signals:
  void SearchResultSignal(ResultsT * result);

private slots:
  void OnSearchPanelItemClicked(int row, int column);
  void OnSearchTextChanged(QString const &);

  /// Called via signal to support multithreading
  void OnSearchResult(ResultsT * result);

  void OnViewportChanged();
  void OnAnimationTimer();
  void OnClearButton();
};

}
