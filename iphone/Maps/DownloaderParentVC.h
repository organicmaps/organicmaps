
#import <UIKit/UIKit.h>
#import "MapsObservers.h"
#import "MapCell.h"
#import "UIKitCategories.h"
#import "ViewController.h"

#include "../../storage/storage_defines.hpp"

#include "../../platform/preferred_languages.hpp"

typedef NS_ENUM(NSUInteger, DownloaderAction)
{
  DownloaderActionDownloadAll,
  DownloaderActionDownloadMap,
  DownloaderActionDownloadCarRouting,
  DownloaderActionDeleteAll,
  DownloaderActionDeleteMap,
  DownloaderActionDeleteCarRouting,
  DownloaderActionCancelDownloading,
  DownloaderActionZoomToCountry
};

using namespace storage;

@interface DownloaderParentVC : ViewController <MapCellDelegate, UIActionSheetDelegate, UIAlertViewDelegate, UITableViewDataSource, UITableViewDelegate>

- (NSString *)formattedMapSize:(size_t)size;

- (BOOL)canDownloadSelectedMap;
- (UIActionSheet *)actionSheetToCancelDownloadingSelectedMap;
- (UIActionSheet *)actionSheetToPerformActionOnSelectedMap;

@property (nonatomic) UITableView * tableView;

@property (nonatomic) NSInteger selectedPosition;

@property (nonatomic) TMapOptions selectedInActionSheetOptions;
@property (nonatomic) NSMutableDictionary * actionSheetActions;

// virtual
- (NSString *)parentTitle;
- (NSString *)selectedMapName;
- (size_t)selectedMapSizeWithOptions:(TMapOptions)options;
- (TStatus)selectedMapStatus;
- (TMapOptions)selectedMapOptions;
- (void)performAction:(DownloaderAction)action withSizeCheck:(BOOL)check;

@end
