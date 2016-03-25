#import "LocationManager.h"
#import "LocationPredictor.h"
#import "MWMViewController.h"
#import <MyTargetSDKCorp/MTRGNativeAppwallAd.h>

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "indexer/map_style.hpp"

namespace search { struct AddressInfo; }

@class MWMMapViewControlsManager;
@class MWMAPIBar;

@interface MapViewController : MWMViewController <LocationObserver>
{
  LocationPredictor * m_predictor;
}

// called when app is terminated by system
- (void)onTerminate;
- (void)onEnterForeground;
- (void)onEnterBackground;
- (void)onGetFocus:(BOOL)isOnFocus;

- (void)setMapStyle:(MapStyle)mapStyle;

- (void)updateStatusBarStyle;

- (void)showAPIBar;

- (void)performAction:(NSString *)action;

- (void)openMigration;
- (void)openBookmarks;
- (void)openMapsDownloader;
- (void)openEditor;
- (void)showReportController;

- (void)refreshAd;

- (void)initialize;

@property (nonatomic) MTRGNativeAppwallAd * appWallAd;
@property (nonatomic, readonly) BOOL isAppWallAdActive;

@property (nonatomic, readonly) MWMMapViewControlsManager * controlsManager;
@property (nonatomic) m2::PointD restoreRouteDestination;
@property (nonatomic) MWMAPIBar * apiBar;

@end
