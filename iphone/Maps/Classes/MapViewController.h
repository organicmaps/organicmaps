#import "MWMMapDownloaderMode.h"
#import "MWMViewController.h"
#import "MWMMyPositionMode.h"

@class MWMMapViewControlsManager;
@class EAGLView;
@class MWMMapDownloadDialog;
@class BookmarksCoordinator;
@class SearchOnMapManager;
@class TrackRecordingManager;
@class SideButtonsArea;
@class WidgetsArea;
@class TrafficButtonArea;
@class PlacePageArea;

@protocol MWMLocationModeListener;

@interface MapViewController : MWMViewController

+ (MapViewController *_Nullable)sharedController;
- (void)addListener:(id<MWMLocationModeListener>_Nonnull)listener;
- (void)removeListener:(id<MWMLocationModeListener>_Nonnull)listener;

// called when app is terminated by system
- (void)onTerminate;
- (void)onGetFocus:(BOOL)isOnFocus;

- (void)updateStatusBarStyle;

- (void)migrateOAuthCredentials;

- (void)performAction:(NSString *_Nonnull)action;

- (void)openMenu;
- (void)openSettings;
- (void)openMapsDownloader:(MWMMapDownloaderMode)mode;
- (void)openEditor;
- (void)openBookmarkEditor;
- (void)openFullPlaceDescriptionWithHtml:(NSString *_Nonnull)htmlString;
- (void)openDrivingOptions;
- (void)showTrackRecordingPlacePage;

- (void)setPlacePageTopBound:(CGFloat)bound duration:(double)duration;

+ (void)setViewport:(double)lat lon:(double)lon zoomLevel:(int)zoomlevel;

- (void)initialize;
- (void)enableCarPlayRepresentation;
- (void)disableCarPlayRepresentation;

- (void)dismissPlacePage;

@property(nonatomic, readonly, nonnull) MWMMapViewControlsManager * controlsManager;
@property(nonatomic, readonly, nonnull) MWMMapDownloadDialog * downloadDialog;
@property(nonatomic, readonly, nonnull) BookmarksCoordinator * bookmarksCoordinator;
@property(nonatomic, readonly, nonnull) SearchOnMapManager * searchManager;
@property(nonatomic, readonly, nonnull) TrackRecordingManager * trackRecordingManager;

@property(nonatomic) MWMMyPositionMode currentPositionMode;
@property(strong, nonatomic, nonnull) IBOutlet EAGLView * mapView;
@property(strong, nonatomic, nonnull) IBOutlet UIView * controlsView;
@property(nonatomic, nonnull) UIView * searchContainer;

@property (weak, nonatomic) IBOutlet SideButtonsArea * sideButtonsArea;
@property (weak, nonatomic) IBOutlet WidgetsArea * widgetsArea;
@property (weak, nonatomic) IBOutlet TrafficButtonArea * trafficButtonArea;
@property (weak, nonatomic) IBOutlet PlacePageArea * placePageArea;

@end
