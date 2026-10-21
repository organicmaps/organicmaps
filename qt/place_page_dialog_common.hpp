#pragma once

#include <QtWidgets/QWidget>

#include <string_view>

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
  PlacePageDialogCommon(QWidget * parent, qt::DrawWidget * drawWidget, place_page::Info const & info,
                        QList<QAction *> const & actions);

protected:
  qt::DrawWidget * GetDrawWidget() const { return m_drawWidget; }
  QVBoxLayout * GetContentLayout() const { return m_contentLayout; }

private:
  qt::DrawWidget * m_drawWidget;
  QVBoxLayout * m_contentLayout;
};

namespace place_page_dialog
{
inline QString MakeHtmlLink(QString const & url, QString const & text)
{
  return QString("<a href=\"%1\">%2</a>").arg(url.toHtmlEscaped(), text.toHtmlEscaped());
}

inline QString ShortDescription(std::string_view description)
{
  if (auto const end = description.find("</p>"); description.starts_with("<p>") && end != std::string_view::npos)
    description = description.substr(3, end - 3);

  size_t constexpr kMaxBytes = 500;
  if (description.size() <= kMaxBytes)
    return QString::fromUtf8(description.data(), description.size());

  // Keep the preview byte-limited, reserving three bytes for the ellipsis and preserving UTF-8 code points.
  auto end = kMaxBytes - 3;
  while ((static_cast<unsigned char>(description[end]) & 0xC0) == 0x80)
    --end;
  return QString::fromUtf8(description.data(), end) + QStringLiteral("…");
}

// Builds a toolbar with shared route + edit actions and a place-specific Share menu.
QToolBar * createActionToolBar(QWidget * parent, qt::DrawWidget * drawWidget, place_page::Info const & info,
                               QList<QAction *> const & actions);

// Adds a key/value row to `grid` and increments `row`.
// A nonempty URL makes the displayed value a link to that URL.
QLabel * addEntry(QGridLayout * grid, int & row, QString const & key, QString const & value, QString const & url = {});

// If `info` has route refs, adds a "Routes" row to `grid` with a combo box that
// shows the selected route's transit view via DrawWidget::GetFramework().ShowRouteTransit().
// No-op when there are no routes.
void addRoutesRow(QGridLayout * grid, int & row, qt::DrawWidget * drawWidget, place_page::Info const & info);

// Adds a "Coordinates" row whose value cycles (via a button) through the formats available at the
// place, mirroring the mobile place page. Reuses the place_page coordinate registry and persists the
// selected format id across sessions (see CoordinatesFormat in map/place_page_info.hpp).
void addCoordinatesRow(QGridLayout * grid, int & row, place_page::Info const & info);
}  // namespace place_page_dialog
