
#import "DownloaderParentVC.h"
#import "CustomAlertView.h"
#import "DiskFreeSpace.h"
#import "Statistics.h"
#import "Reachability.h"
#import "MapsAppDelegate.h"

@interface DownloaderParentVC ()

@end

@implementation DownloaderParentVC

- (id)init
{
  self = [super initWithStyle:UITableViewStyleGrouped];
  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];

  self.tableView.separatorStyle = UITableViewCellSeparatorStyleNone;

  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(outOfDateCountriesCountChanged:) name:MapsStatusChangedNotification object:nil];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (buttonIndex != alertView.cancelButtonIndex)
  {
    if (self.selectedInActionSheetOptions == TMapOptions::EMapOnly)
      [self performAction:DownloaderActionDownloadMap];
    else if (self.selectedInActionSheetOptions == TMapOptions::ECarRouting)
      [self performAction:DownloaderActionDownloadCarRouting];
    else if (self.selectedInActionSheetOptions == TMapOptions::EMapWithCarRouting)
      [self performAction:DownloaderActionDownloadAll];
  }
}

- (BOOL)allButtonsInActionSheetAreAboutDownloading:(TStatus const)status
{
  return status == TStatus::ENotDownloaded;
}

#pragma mark - Virtual methods

- (void)performAction:(DownloaderAction)action {}
- (NSString *)parentTitle { return nil; }
- (NSString *)selectedMapName { return nil; }
- (NSString *)selectedMapGuideName { return nil; }
- (size_t)selectedMapSizeWithOptions:(TMapOptions)options { return 0; }
- (TStatus)selectedMapStatus { return TStatus::EUnknown; }
- (TMapOptions)selectedMapOptions { return TMapOptions::EMapOnly; }

#pragma mark - Public methods for successors

#define MB (1024 * 1024)

- (NSString *)formattedMapSize:(size_t)size
{
  NSString * sizeString;
  if (size > MB)
    sizeString = [NSString stringWithFormat:@"%ld %@", (size + 512 * 1024) / MB, L(@"mb")];
  else
    sizeString = [NSString stringWithFormat:@"%ld %@", (size + 1023) / 1024, L(@"kb")];
  return [sizeString uppercaseString];
}

- (BOOL)canDownloadSelectedMap
{
  size_t const size = [self selectedMapSizeWithOptions:self.selectedInActionSheetOptions];
  NSString * name = [self selectedMapName];

  Reachability * reachability = [Reachability reachabilityForInternetConnection];
  if ([reachability isReachable])
  {
    if ([reachability isReachableViaWWAN] && size > 50 * MB)
    {
      NSString * title = [NSString stringWithFormat:L(@"no_wifi_ask_cellular_download"), name];
      [[[CustomAlertView alloc] initWithTitle:title message:nil delegate:self cancelButtonTitle:L(@"cancel") otherButtonTitles:L(@"use_cellular_data"), nil] show];
    }
    else
      return YES;
  }
  else
  {
    [[[CustomAlertView alloc] initWithTitle:L(@"no_internet_connection_detected") message:L(@"use_wifi_recommendation_text") delegate:nil cancelButtonTitle:L(@"ok") otherButtonTitles:nil] show];
  }
  return NO;
}

- (void)openGuideWithInfo:(const guides::GuideInfo &)info
{
  string const lang = languages::GetCurrentNorm();
  NSURL * guideUrl = [NSURL URLWithString:[NSString stringWithUTF8String:info.GetAppID().c_str()]];
  [[Statistics instance] logEvent:@"Open Guide Country" withParameters:@{@"Country Name" : [self selectedMapName]}];
  UIApplication * application = [UIApplication sharedApplication];
  if ([application canOpenURL:guideUrl])
  {
    [application openURL:guideUrl];
    [[Statistics instance] logEvent:@"Open Guide Button" withParameters:@{@"Guide downloaded" : @"YES"}];
  }
  else
  {
    [application openURL:[NSURL URLWithString:[NSString stringWithUTF8String:info.GetURL().c_str()]]];
    [[Statistics instance] logEvent:@"Open Guide Button" withParameters:@{@"Guide downloaded" : @"NO"}];
  }
}

#define ROUTING_SYMBOL @"\xF0\x9F\x9A\x97"

- (UIActionSheet *)actionSheetToPerformActionOnSelectedMap
{
  TStatus const status = [self selectedMapStatus];
  TMapOptions const options = [self selectedMapOptions];

  [self.actionSheetActions removeAllObjects];

  NSString * title;
  if ([self allButtonsInActionSheetAreAboutDownloading:status])
    title = [NSString stringWithFormat:@"%@ %@", L(@"download").uppercaseString, [self actionSheetTitle]];
  else
    title = [self actionSheetTitle];

  UIActionSheet * actionSheet = [[UIActionSheet alloc] initWithTitle:title delegate:self cancelButtonTitle:nil destructiveButtonTitle:nil otherButtonTitles:nil];

  if (status == TStatus::EOnDisk || status == TStatus::EOnDiskOutOfDate)
  {
    if ([self selectedMapGuideName])
      [self addButtonWithTitle:[self selectedMapGuideName] action:DownloaderActionShowGuide toActionSheet:actionSheet];

    [self addButtonWithTitle:L(@"zoom_to_country") action:DownloaderActionZoomToCountry toActionSheet:actionSheet];
  }

  if (status == TStatus::ENotDownloaded || status == TStatus::EOutOfMemFailed || status == TStatus::EDownloadFailed)
  {
    NSString * size = [self formattedMapSize:[self selectedMapSizeWithOptions:TMapOptions::EMapOnly]];
    NSString * title;
    if ([self allButtonsInActionSheetAreAboutDownloading:status])
      title = [NSString stringWithFormat:@"%@, %@", L(@"downloader_map_only"), size];
    else
      title = [NSString stringWithFormat:@"%@, %@", L(@"downloader_download_map"), size];
    [self addButtonWithTitle:title action:DownloaderActionDownloadMap toActionSheet:actionSheet];
  }

  if (status == TStatus::ENotDownloaded || status == TStatus::EOutOfMemFailed || status == TStatus::EDownloadFailed)
  {
    NSString * size = [self formattedMapSize:[self selectedMapSizeWithOptions:TMapOptions::EMapWithCarRouting]];
    NSString * title;
    if ([self allButtonsInActionSheetAreAboutDownloading:status])
      title = [NSString stringWithFormat:@"%@%@, %@", L(@"downloader_map_and_routing"), ROUTING_SYMBOL, size];
    else
      title = [NSString stringWithFormat:@"%@%@, %@", L(@"downloader_download_map_and_routing"), ROUTING_SYMBOL, size];

    [self addButtonWithTitle:title action:DownloaderActionDownloadAll toActionSheet:actionSheet];
  }

  if (status == TStatus::EOnDiskOutOfDate && options == TMapOptions::EMapWithCarRouting)
  {
    NSString * size = [self formattedMapSize:[self selectedMapSizeWithOptions:TMapOptions::EMapWithCarRouting]];
    NSString * title = [NSString stringWithFormat:@"%@%@, %@", L(@"downloader_update_map_and_routing"), ROUTING_SYMBOL, size];
    [self addButtonWithTitle:title action:DownloaderActionDownloadAll toActionSheet:actionSheet];
  }

  if (status == TStatus::EOnDisk && options == TMapOptions::EMapOnly)
  {
    NSString * size = [self formattedMapSize:[self selectedMapSizeWithOptions:TMapOptions::ECarRouting]];
    NSString * title = [NSString stringWithFormat:@"%@, %@", L(@"downloader_download_routing"), size];
    [self addButtonWithTitle:title action:DownloaderActionDownloadCarRouting toActionSheet:actionSheet];
  }

  if (status == TStatus::EOnDiskOutOfDate && options == TMapOptions::EMapOnly)
  {
    NSString * size = [self formattedMapSize:[self selectedMapSizeWithOptions:TMapOptions::EMapOnly]];
    NSString * title = [NSString stringWithFormat:@"%@, %@", L(@"downloader_update_map"), size];
    [self addButtonWithTitle:title action:DownloaderActionDownloadMap toActionSheet:actionSheet];
  }

  if (status == TStatus::EOnDisk || status == TStatus::EOnDiskOutOfDate)
  {
    if (options == TMapOptions::EMapWithCarRouting)
      [self addButtonWithTitle:L(@"downloader_delete_routing") action:DownloaderActionDeleteCarRouting toActionSheet:actionSheet];

    [self addButtonWithTitle:L(@"downloader_delete_map") action:DownloaderActionDeleteMap toActionSheet:actionSheet];
    actionSheet.destructiveButtonIndex = actionSheet.numberOfButtons - 1;
  }

  if (status == TStatus::EDownloading || status == TStatus::EInQueue)
  {
    [self addButtonWithTitle:L(@"cancel_download") action:DownloaderActionCancelDownloading toActionSheet:actionSheet];
    actionSheet.destructiveButtonIndex = actionSheet.numberOfButtons - 1;
  }

  if (!IPAD)
  {
    [actionSheet addButtonWithTitle:L(@"cancel")];
    actionSheet.cancelButtonIndex = actionSheet.numberOfButtons - 1;
  }

  return actionSheet;
}

- (UIActionSheet *)actionSheetToCancelDownloadingSelectedMap
{
  [self.actionSheetActions removeAllObjects];
  self.actionSheetActions[@0] = @(DownloaderActionCancelDownloading);

  return [[UIActionSheet alloc] initWithTitle:[self actionSheetTitle] delegate:self cancelButtonTitle:L(@"cancel") destructiveButtonTitle:L(@"cancel_download") otherButtonTitles:nil];
}

#pragma mark -

- (NSString *)actionSheetTitle
{
  if ([self parentTitle])
    return [NSString stringWithFormat:@"%@, %@", [self selectedMapName].uppercaseString, [self parentTitle].uppercaseString];
  else
    return [self selectedMapName].uppercaseString;
}

- (void)addButtonWithTitle:(NSString *)title action:(DownloaderAction)action toActionSheet:(UIActionSheet *)actionSheet
{
  [actionSheet addButtonWithTitle:title];
  self.actionSheetActions[@(actionSheet.numberOfButtons - 1)] = @(action);
}

- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (buttonIndex != actionSheet.cancelButtonIndex)
  {
    DownloaderAction const action = (DownloaderAction)[self.actionSheetActions[@(buttonIndex)] integerValue];
    switch (action)
    {
      case DownloaderActionDownloadAll:
      case DownloaderActionDeleteAll:
        self.selectedInActionSheetOptions = TMapOptions::EMapWithCarRouting;
        break;

      case DownloaderActionDownloadMap:
      case DownloaderActionDeleteMap:
        self.selectedInActionSheetOptions = TMapOptions::EMapOnly;
        break;

      case DownloaderActionDownloadCarRouting:
      case DownloaderActionDeleteCarRouting:
        self.selectedInActionSheetOptions = TMapOptions::ECarRouting;
        break;

      default:
        break;
    }

    [self performAction:action];
  }
}

#pragma mark -

- (NSMutableDictionary *)actionSheetActions
{
  if (!_actionSheetActions)
    _actionSheetActions = [[NSMutableDictionary alloc] init];
  return _actionSheetActions;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end
