#pragma once

#include <QWidget>

namespace search { class Result; }
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
