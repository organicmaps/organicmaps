#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>

#include <string>

class FeatureType;

namespace search
{
struct AddressInfo;
}

class FeatureInfoDialog : public QDialog
{
  Q_OBJECT
public:
  FeatureInfoDialog(QWidget * parent, FeatureType & ft, search::AddressInfo const & address,
                    std::string const & locale);

private:
  void AddElems(QGridLayout & layout, int row, int col, QWidget * widget)
  {
    layout.addWidget(widget, row, col);
  }

  template <typename... Args>
  void AddElems(QGridLayout & layout, int row, int col, QWidget * widget, Args *... args)
  {
    AddElems(layout, row, col, widget);
    AddElems(layout, row, col + 1, args...);
  }

  template <typename... Args>
  void AddRow(QGridLayout & layout, Args *... args)
  {
    AddElems(layout, m_numRows /* row */, 0 /* col */, args...);
    ++m_numRows;
  }

  int m_numRows = 0;
};
