#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMMapViewControlsManager.h"
#import "MWMPlacePageEntity.h"
#import "MWMReportBaseController.h"
#import "Statistics.h"

#include "indexer/osm_editor.hpp"

@implementation MWMReportBaseController
{
  m2::PointD m_point;
}

- (void)configNavBar
{
  self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:L(@"editor_report_problem_send_button")
                                            style:UIBarButtonItemStylePlain target:self action:@selector(send)];
}

- (void)send
{
  [self doesNotRecognizeSelector:_cmd];
}

- (void)sendNote:(string const &)note
{
  NSAssert(!note.empty(), @"String can't be empty!");
  auto const & featureID = MapsAppDelegate.theApp.mapViewController.controlsManager.placePageEntity.info.GetID();
  auto const latLon = ToLatLon(m_point);
  osm::Editor::Instance().CreateNote(m_point, note);
  [Statistics logEvent:kStatEditorProblemReport withParameters:@{kStatEditorMWMName : @(featureID.GetMwmName().c_str()),
                                                                 kStatEditorMWMVersion : @(featureID.GetMwmVersion()),
                                                                 kStatProblem : @(note.c_str()),
                                                                 kStatLat : @(latLon.lat),
                                                                 kStatLon : @(latLon.lon)}];
  [self.navigationController popToRootViewControllerAnimated:YES];
}

- (void)setPoint:(m2::PointD const &)point
{
  m_point = point;
}

- (m2::PointD const &)point
{
  return m_point;
}

@end
