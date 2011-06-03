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
  vector<search::Result> m_results;

  Q_OBJECT

private:
  void OnSearchResult(search::Result const & result);
protected:
  virtual void showEvent(QShowEvent *);
  virtual void hideEvent(QHideEvent *);

public:
  explicit SearchPanel(DrawWidget * drawWidget, QWidget * parent);

protected slots:
  void OnSearchPanelItemClicked(int row, int column);
  void OnSearchTextChanged(QString const &);
};

}
