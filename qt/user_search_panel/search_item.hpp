#pragma once

#include <QtWidgets/QWidget>

class QLabel;

namespace search
{
class Result;
}

namespace qt
{

// TODO (@zagto): This should be implemented as a Delegate, not a Widget. While using a Widget
// allows using QLayout and QLabel for easier layouting, it has disadventages:
//  - manual updating of sizeHint is a hack and sometimes causes layout issues
//  - The colors of the labels are not updated properly depending on selected, active, states etc.
//    of the item
//  - bad performance for large lists
class SearchItem : public QWidget
{
  Q_OBJECT

public:
  SearchItem(search::Result const & result, QWidget * parent);

private:
  QLabel * CreateLabelWithSearchHighlight(search::Result const & result);
};

}  // namespace qt
