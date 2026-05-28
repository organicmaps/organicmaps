#include "qt/place_page_dialog_common.hpp"

#include "qt/draw_widget.hpp"

#include "map/framework.hpp"
#include "map/place_page_info.hpp"
#include "map/routing_mark.hpp"

#include "indexer/feature_decl.hpp"

#include "kml/types.hpp"

#include <QtGui/QIcon>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>

PlacePageDialogCommon::PlacePageDialogCommon(QWidget * parent, qt::DrawWidget * drawWidget,
                                             place_page::Info const & info)
  : QWidget(parent)
  , m_drawWidget(drawWidget)
{
  QVBoxLayout * layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  layout->addWidget(place_page_dialog::createActionToolBar(this, drawWidget, info));

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
QToolBar * createActionToolBar(QWidget * parent, qt::DrawWidget * drawWidget, place_page::Info const & info)
{
  QToolBar * toolBar = new QToolBar(parent);
  toolBar->setIconSize(QSize(20, 20));

  m2::PointD const mercator = info.GetMercator();

  QAction * fromAction = toolBar->addAction(QIcon(":/navig64/point-start.png"), "Route From");
  fromAction->setToolTip("Route From");
  QObject::connect(fromAction, &QAction::triggered, parent,
                   [drawWidget, mercator] { drawWidget->RoutePointFromPlace(RouteMarkType::Start, mercator); });

  QAction * stopAction = toolBar->addAction(QIcon(":/navig64/point-intermediate.png"), "Add Stop");
  stopAction->setToolTip("Add Stop");
  QObject::connect(stopAction, &QAction::triggered, parent,
                   [drawWidget, mercator] { drawWidget->RoutePointFromPlace(RouteMarkType::Intermediate, mercator); });

  QAction * toAction = toolBar->addAction(QIcon(":/navig64/point-finish.png"), "Route To");
  toAction->setToolTip("Route To");
  QObject::connect(toAction, &QAction::triggered, parent,
                   [drawWidget, mercator] { drawWidget->RoutePointFromPlace(RouteMarkType::Finish, mercator); });

  if (info.IsTrack())
  {
    kml::TrackId const trackId = info.GetTrackId();
    QAction * alongAction = toolBar->addAction("Route Along Track");
    QObject::connect(alongAction, &QAction::triggered, parent,
                     [drawWidget, trackId] { drawWidget->RouteAlongTrack(trackId); });
  }

  if (info.ShouldShowEditPlace())
  {
    FeatureID const featureId = info.GetID();
    QAction * editAction = toolBar->addAction("Edit Place");
    QObject::connect(editAction, &QAction::triggered, parent,
                     [drawWidget, featureId] { drawWidget->EditPlace(featureId); });
  }

  return toolBar;
}

QLabel * addEntry(QGridLayout * grid, int & row, std::string const & key, std::string const & value, bool isLink)
{
  grid->addWidget(new QLabel(QString::fromStdString(key)), row, 0);
  QLabel * label = new QLabel(QString::fromStdString(value));
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  label->setWordWrap(true);
  if (isLink)
  {
    label->setOpenExternalLinks(true);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    label->setText(QString::fromStdString("<a href=\"" + value + "\">" + value + "</a>"));
  }
  grid->addWidget(label, row++, 1);
  return label;
}

void addRoutesRow(QGridLayout * grid, int & row, qt::DrawWidget * drawWidget, place_page::Info const & info)
{
  auto const & routes = info.GetRoutes();
  if (routes.empty())
    return;

  grid->addWidget(new QLabel("Routes"), row, 0);

  QComboBox * routesCombo = new QComboBox();
  // Placeholder so opening the widget does not auto-select (and trigger) the first route.
  routesCombo->setPlaceholderText("Select a route…");
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
}  // namespace place_page_dialog
