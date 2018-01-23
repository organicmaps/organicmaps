#import "MWMMapDownloaderMode.h"
#import "MWMViewController.h"

@class MWMWelcomePageController;
@class MWMMapViewControlsManager;
@class MWMAPIBar;
@class MWMPlacePageData;

@interface MapViewController : MWMViewController

+ (MapViewController *)controller;

// called when app is terminated by system
- (void)onTerminate;
- (void)onGetFocus:(BOOL)isOnFocus;

- (void)updateStatusBarStyle;

- (void)performAction:(NSString *)action;

- (void)openMigration;
- (void)openBookmarks;
- (void)openMapsDownloader:(MWMMapDownloaderMode)mode;
- (void)openEditor;
- (void)openHotelFacilities;
- (void)openBookmarkEditorWithData:(MWMPlacePageData *)data;

- (void)setPlacePageTopBound:(CGFloat)bound;

- (void)initialize;

@property(nonatomic, readonly) MWMMapViewControlsManager * controlsManager;
@property(nonatomic) MWMAPIBar * apiBar;
@property(nonatomic) MWMWelcomePageController * welcomePageController;

@end
