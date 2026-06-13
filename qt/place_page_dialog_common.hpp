#pragma once

#include <QtWidgets/QWidget>

#include <string>

class QGridLayout;
class QLabel;
class QToolBar;
class QVBoxLayout;

namespace place_page
{
class Info;
}

namespace qt
{
class DrawWidget;
}

// Shared boilerplate for both place-page widget variants:
// outer vertical layout = action toolbar + QScrollArea wrapping a content layout.
// Subclasses populate GetContentLayout() with their entries and end with
// contentLayout->addStretch() to anchor everything to the top.
class PlacePageDialogCommon : public QWidget
{
  Q_OBJECT

public:
  PlacePageDialogCommon(QWidget * parent, qt::DrawWidget * drawWidget, place_page::Info const & info);

protected:
  qt::DrawWidget * GetDrawWidget() const { return m_drawWidget; }
  QVBoxLayout * GetContentLayout() const { return m_contentLayout; }

private:
  qt::DrawWidget * m_drawWidget;
  QVBoxLayout * m_contentLayout;
};

namespace place_page_dialog
{
// Builds a toolbar with route + edit actions wired to `drawWidget`.
// Action handlers capture only the values they need from `info`, so the
// returned toolbar stays valid even after `info` is invalidated.
QToolBar * createActionToolBar(QWidget * parent, qt::DrawWidget * drawWidget, place_page::Info const & info);

// Adds a key/value row to `grid` and increments `row`.
// If `isLink`, wraps the value in <a href> markup.
QLabel * addEntry(QGridLayout * grid, int & row, std::string const & key, std::string const & value,
                  bool isLink = false);

// If `info` has route refs, adds a "Routes" row to `grid` with a combo box that
// shows the selected route's transit view via DrawWidget::GetFramework().ShowRouteTransit().
// No-op when there are no routes.
void addRoutesRow(QGridLayout * grid, int & row, qt::DrawWidget * drawWidget, place_page::Info const & info);

// Adds a "Coordinates" row whose value cycles (via a button) through the formats available at the
// place, mirroring the mobile place page. Reuses the place_page coordinate registry and persists the
// selected format id across sessions (see CoordinatesFormat in map/place_page_info.hpp).
void addCoordinatesRow(QGridLayout * grid, int & row, place_page::Info const & info);
}  // namespace place_page_dialog
