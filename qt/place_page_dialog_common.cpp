#include "qt/place_page_dialog_common.hpp"

#include "qt/draw_widget.hpp"

#include "map/place_page_info.hpp"
#include "map/routing_mark.hpp"

#include "indexer/feature_decl.hpp"

#include "kml/types.hpp"

#include <QtGui/QIcon>
#include <QtWidgets/QToolBar>

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
}  // namespace place_page_dialog
