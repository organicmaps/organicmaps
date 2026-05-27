#pragma once

class QToolBar;
class QWidget;

namespace place_page
{
class Info;
}

namespace qt
{
class DrawWidget;
}

namespace place_page_dialog
{
// Builds a toolbar with route + edit actions wired to `drawWidget`.
// Action handlers capture only the values they need from `info`, so the
// returned toolbar stays valid even after `info` is invalidated.
QToolBar * createActionToolBar(QWidget * parent, qt::DrawWidget * drawWidget, place_page::Info const & info);
}  // namespace place_page_dialog
