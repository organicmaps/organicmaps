
#import <UIKit/UIKit.h>
#import "MapsObservers.h"
#import "MapCell.h"
#import "ViewController.h"

#include "storage/storage_defines.hpp"
#include "platform/preferred_languages.hpp"

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

- (BOOL)canDownloadSelectedMap;
- (UIActionSheet *)actionSheetToCancelDownloadingSelectedMap;
- (UIActionSheet *)actionSheetToPerformActionOnSelectedMap;

@property (nonatomic) UITableView * tableView;

// CountryTree methods accept int. It should be enough to store all countries.
@property (nonatomic) int selectedPosition;

@property (nonatomic) MapOptions selectedInActionSheetOptions;
@property (nonatomic) NSMutableDictionary * actionSheetActions;

// virtual
- (NSString *)parentTitle;
- (NSString *)selectedMapName;
- (uint64_t)selectedMapSizeWithOptions:(MapOptions)options;
- (TStatus)selectedMapStatus;
- (MapOptions)selectedMapOptions;
- (void)performAction:(DownloaderAction)action withSizeCheck:(BOOL)check;

@end
