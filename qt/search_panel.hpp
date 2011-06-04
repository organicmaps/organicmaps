#pragma once

#include "../search/result.hpp"

#include "../std/vector.hpp"

#include <QWidget>

class QTableWidget;
class QLineEdit;

namespace qt
{

class DrawWidget;

class SearchPanel : public QWidget
{
  DrawWidget * m_pDrawWidget;
  QTableWidget * m_pTable;
  QLineEdit * m_pEditor;

  /// Stores current search results
  vector<search::Result *> m_results;

  Q_OBJECT

signals:
  void SearchResultSignal(search::Result * result);

private:
  void SearchResultThreadFunc(search::Result const & result);

public:
  explicit SearchPanel(DrawWidget * drawWidget, QWidget * parent);
  ~SearchPanel();

protected slots:
  void OnSearchPanelItemClicked(int row, int column);
  void OnSearchTextChanged(QString const &);
  /// Called via signal to support multithreading
  void OnSearchResult(search::Result * result);
};

}
