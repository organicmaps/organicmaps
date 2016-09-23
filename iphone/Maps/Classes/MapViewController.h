#import "MWMMapDownloaderTypes.h"
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

- (void)initialize;

@property(nonatomic, readonly) MWMMapViewControlsManager * controlsManager;
@property(nonatomic) MWMAPIBar * apiBar;

@end
