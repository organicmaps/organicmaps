#pragma once

#include "../search/result.hpp"

#include "../std/vector.hpp"

#include <QWidget>

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

  /// Stores current search results
  vector<search::Result *> m_results;
  int volatile m_queryId;

  Q_OBJECT

signals:
  void SearchResultSignal(search::Result * result, int queryId);

private:
  void SearchResultThreadFunc(search::Result const & result, int queryId);
  virtual void showEvent(QShowEvent *);
  virtual void hideEvent(QHideEvent *);

public:
  explicit SearchPanel(DrawWidget * drawWidget, QWidget * parent);
  ~SearchPanel();

private slots:
  void OnSearchPanelItemClicked(int row, int column);
  void OnSearchTextChanged(QString const &);
  /// Called via signal to support multithreading
  void OnSearchResult(search::Result * result, int queryId);
  void OnViewportChanged();
  void OnAnimationTimer();
  void OnClearButton();
};

}
