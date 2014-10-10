
#import <UIKit/UIKit.h>
#import "MapsObservers.h"
#import "MapCell.h"
#import "UIKitCategories.h"

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
  DownloaderActionZoomToCountry,
  DownloaderActionShowGuide
};

using namespace storage;

@interface DownloaderParentVC : UITableViewController <MapCellDelegate, UIActionSheetDelegate, UIAlertViewDelegate>

- (NSString *)formattedMapSize:(size_t)size;
- (void)openGuideWithInfo:(guides::GuideInfo const &)info;

- (BOOL)canDownloadSelectedMap;
- (UIActionSheet *)actionSheetToDeleteSelectedMap;
- (UIActionSheet *)actionSheetToCancelDownloadingSelectedMap;
- (UIActionSheet *)actionSheetToPerformActionOnSelectedMap;

@property (nonatomic) NSInteger selectedPosition;

@property (nonatomic) TMapOptions selectedInActionSheetOptions;
@property (nonatomic) NSMutableDictionary * actionSheetActions;

// virtual
- (NSString *)parentTitle;
- (NSString *)selectedMapName;
- (NSString *)selectedMapGuideName;
- (size_t)selectedMapSizeWithOptions:(TMapOptions)options;
- (TStatus)selectedMapStatus;
- (TMapOptions)selectedMapOptions;
- (void)performAction:(DownloaderAction)action;

@end
