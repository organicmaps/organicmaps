#include "qt/place_page_dialog_common.hpp"
#include "qt/qt_common/translations.hpp"

#include "qt/draw_widget.hpp"

#include "map/framework.hpp"
#include "map/place_page_info.hpp"
#include "map/routing_mark.hpp"

#include "indexer/feature_decl.hpp"

#include "kml/types.hpp"

#include "platform/settings.hpp"

#include <cstdint>
#include <optional>

#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtGui/QGuiApplication>
#include <QtGui/QIcon>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMenu>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

namespace
{
// Persisted in Qt's own settings store. The value is a place_page::CoordinatesFormat stable id - the
// same id space mobile uses, though each platform persists it independently.
char const kCoordinatesFormatSetting[] = "CoordinatesFormat";

int32_t SavedCoordinateFormat()
{
  auto saved = static_cast<int32_t>(place_page::CoordinatesFormat::LatLonDecimal);
  settings::TryGet(kCoordinatesFormatSetting, saved);
  return saved;
}
}  // namespace

PlacePageDialogCommon::PlacePageDialogCommon(QWidget * parent, qt::DrawWidget * drawWidget,
                                             place_page::Info const & info, const QList<QAction *> &actions)
  : QWidget(parent)
  , m_drawWidget(drawWidget)
{
  QVBoxLayout * layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  layout->addWidget(place_page_dialog::createActionToolBar(this, drawWidget, info, actions));

  QScrollArea * scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);
  scrollArea->setFrameShape(QFrame::NoFrame);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  QWidget * content = new QWidget(scrollArea);
  m_contentLayout = new QVBoxLayout(content);

  scrollArea->setWidget(content);
  layout->addWidget(scrollArea);
}

namespace place_page_dialog
{
QToolBar * createActionToolBar(QWidget * parent, qt::DrawWidget * drawWidget, place_page::Info const & info, const QList<QAction *> &actions)
{
  QToolBar * toolBar = new QToolBar(parent);
  toolBar->setIconSize(QSize(20, 20));

  toolBar->addActions(actions);

  // Share: copy the omaps.app link or the full text, or start an email with the location. The core
  // builds the same payload the mobile apps share. Tracks are shared as a file on mobile, and a
  // single-point link would misrepresent them, so they get no Share menu here.
  if (!info.IsTrack())
  {
    QToolButton * shareButton = new QToolButton(toolBar);
    shareButton->setText(qt::Tr("share"));
    shareButton->setToolTip(qt::Tr("share"));
    shareButton->setPopupMode(QToolButton::InstantPopup);

    // The place page lives in a dock that stays alive after the map selection is dropped, unlike the
    // mobile sheets, so the payload is built on demand and only while the selection is still there.
    auto const getShareData = [drawWidget]() -> std::optional<share::Result>
    {
      auto & f = drawWidget->GetFramework();
      if (!f.HasPlacePageInfo())
        return std::nullopt;
      return f.GetShareData(f.GetCurrentPlacePageInfo());
    };

    QMenu * shareMenu = new QMenu(shareButton);
    QObject::connect(shareMenu->addAction(qt::Tr("copy_link")), &QAction::triggered, parent, [getShareData]
    {
      if (auto const d = getShareData())
        QGuiApplication::clipboard()->setText(QString::fromStdString(d->m_url));
    });
    QObject::connect(shareMenu->addAction(qt::Tr("copy_text")), &QAction::triggered, parent, [getShareData]
    {
      if (auto const d = getShareData())
        QGuiApplication::clipboard()->setText(QString::fromStdString(d->m_text));
    });
    QObject::connect(shareMenu->addAction(qt::Tr("email")), &QAction::triggered, parent, [getShareData]
    {
      auto const d = getShareData();
      if (!d)
        return;
      QString subject = qt::Tr("share_place_subject_default");
      if (d->m_isMyPosition)
        subject = qt::Tr("share_my_position");
      else if (!d->m_subjectBasis.empty())
        subject = qt::Tr("share_place_subject").arg(QString::fromStdString(d->m_subjectBasis));

      QUrlQuery query;
      query.addQueryItem("subject", subject);
      query.addQueryItem("body", QString::fromStdString(d->m_text));
      QUrl mailto("mailto:");
      mailto.setQuery(query);
      QDesktopServices::openUrl(mailto);
    });
    shareButton->setMenu(shareMenu);
    toolBar->addWidget(shareButton);
  }

  return toolBar;
}

QLabel * addEntry(QGridLayout * grid, int & row, QString const & key, QString const & value, QString const & url)
{
  grid->addWidget(new QLabel(key), row, 0);
  QLabel * label = new QLabel(value);
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  label->setWordWrap(true);
  if (!url.isEmpty())
  {
    label->setOpenExternalLinks(true);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    label->setText(MakeHtmlLink(url, value));
  }
  grid->addWidget(label, row++, 1);
  return label;
}

void addRoutesRow(QGridLayout * grid, int & row, qt::DrawWidget * drawWidget, place_page::Info const & info)
{
  auto const & routes = info.GetRoutes();
  if (routes.empty())
    return;

  grid->addWidget(new QLabel(qt::Tr("placepage_transit_routes")), row, 0);

  QComboBox * routesCombo = new QComboBox();
  // Placeholder so opening the widget does not auto-select (and trigger) the first route.
  routesCombo->setPlaceholderText(qt::Tr("select_transit_route"));
  for (auto const & r : routes)
  {
    QString const text = QString::fromStdString(r.m_ref);
    QString const tip = QString::fromStdString(r.m_from + (r.m_to.empty() ? "" : " → " + r.m_to));
    routesCombo->addItem(text, QVariant::fromValue<uint32_t>(r.m_relID));
    routesCombo->setItemData(routesCombo->count() - 1, tip, Qt::ToolTipRole);
  }
  QObject::connect(routesCombo, QOverload<int>::of(&QComboBox::activated), routesCombo,
                   [drawWidget, routesCombo](int idx)
  { drawWidget->GetFramework().ShowRouteTransit(routesCombo->itemData(idx).value<uint32_t>()); });

  grid->addWidget(routesCombo, row++, 1);
}

void addCoordinatesRow(QGridLayout * grid, int & row, place_page::Info const & info)
{
  // The place and its region are fixed for this row, so resolve the available formats (with their
  // display strings) once and capture them by value - the handler then just cycles the list and stays
  // valid after `info` is gone (same rule as the toolbar). The region comes straight from the place,
  // with no polygon lookup, to gate the OS Grid etc.
  auto const entries = place_page::GetAvailableCoordinateFormats(info.GetLatLon(), info.GetCountryId());

  grid->addWidget(new QLabel(qt::Tr("placepage_coordinates")), row, 0);

  QLabel * value = new QLabel();
  value->setTextInteractionFlags(Qt::TextSelectableByMouse);  // selectable for copy
  value->setWordWrap(true);

  auto const render = [value, entries](place_page::CoordinatesFormat format)
  {
    for (auto const & e : entries)
      if (e.m_format == format)
        value->setText(QString::fromStdString(e.m_display));
  };
  render(place_page::EffectiveCoordinateFormat(entries, SavedCoordinateFormat()));

  // The button cycles to the next available format and persists the choice, mirroring the tap on
  // mobile. A separate control keeps the value label free for text selection / copy.
  QToolButton * change = new QToolButton();
  change->setText(QString::fromUtf8("↻"));
  change->setToolTip(qt::Tr("switch_coordinates_format"));
  QObject::connect(change, &QToolButton::clicked, value, [entries, render]
  {
    auto const next = place_page::NextCoordinateFormat(entries, SavedCoordinateFormat());
    settings::Set(kCoordinatesFormatSetting, static_cast<int32_t>(next));
    render(next);
  });

  QWidget * cell = new QWidget();
  QHBoxLayout * cellLayout = new QHBoxLayout(cell);
  cellLayout->setContentsMargins(0, 0, 0, 0);
  cellLayout->addWidget(value, 1);
  cellLayout->addWidget(change, 0);
  grid->addWidget(cell, row++, 1);
}
}  // namespace place_page_dialog
