#import "MWMMapDownloaderMode.h"
#import "MWMViewController.h"

@class MWMWelcomePageController;
@class MWMMapViewControlsManager;
@class MWMAPIBar;
@class MWMPlacePageData;

@interface MapViewController : MWMViewController

+ (MapViewController *)sharedController;

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
- (void)openFullPlaceDescriptionWithHtml:(NSString *)htmlString;
- (void)showUGCAuth;
- (void)showBookmarksLoadedAlert:(UInt64)categoryId;
- (void)openCatalogAnimated:(BOOL)animated;
- (void)openCatalogDeeplink:(NSURL * _Nullable)deeplinkUrl animated:(BOOL)animated;
- (void)searchText:(NSString *)text;

- (void)showRemoveAds;
- (void)setPlacePageTopBound:(CGFloat)bound;

- (void)initialize;

@property(nonatomic, readonly) MWMMapViewControlsManager * controlsManager;
@property(nonatomic) MWMAPIBar * apiBar;
@property(nonatomic) MWMWelcomePageController * welcomePageController;
@property(nonatomic, getter=isLaunchByDeepLink) BOOL launchByDeepLink;

@end
