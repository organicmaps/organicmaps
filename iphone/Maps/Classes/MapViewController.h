#import "MWMMapDownloaderTypes.h"
#import "MWMPageController.h"
#import "MWMViewController.h"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "indexer/map_style.hpp"

namespace search
{
struct AddressInfo;
}

@class MWMMapViewControlsManager;
@class MWMAPIBar;
@class MWMPlacePageData;

@interface MapViewController : MWMViewController

+ (MapViewController *)controller;

// called when app is terminated by system
- (void)onTerminate;
- (void)onGetFocus:(BOOL)isOnFocus;

- (void)updateStatusBarStyle;

- (void)showAPIBar;

- (void)performAction:(NSString *)action;

- (void)openMigration;
- (void)openBookmarks;
- (void)openMapsDownloader:(mwm::DownloaderMode)mode;
- (void)openEditor;
- (void)openBookmarkEditorWithData:(MWMPlacePageData *)data;

- (void)initialize;

@property(nonatomic, readonly) MWMMapViewControlsManager * controlsManager;
@property(nonatomic) MWMAPIBar * apiBar;
@property(nonatomic) MWMPageController * pageViewController;

@end
