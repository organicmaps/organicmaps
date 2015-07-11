
#import "DownloaderParentVC.h"
#import "DiskFreeSpace.h"
#import "Statistics.h"
#import "MWMAlertViewController.h"

#include "platform/platform.hpp"

@implementation DownloaderParentVC

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self.view addSubview:self.tableView];
}

- (UITableView *)tableView
{
  if (!_tableView) {
    _tableView = [[UITableView alloc] initWithFrame:self.view.bounds style:UITableViewStyleGrouped];
    _tableView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    _tableView.delegate = self;
    _tableView.dataSource = self;
    _tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
  }
  return _tableView;
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (buttonIndex != alertView.cancelButtonIndex)
  {
    if (self.selectedInActionSheetOptions == TMapOptions::EMap)
      [self performAction:DownloaderActionDownloadMap withSizeCheck:NO];
    else if (self.selectedInActionSheetOptions == TMapOptions::ECarRouting)
      [self performAction:DownloaderActionDownloadCarRouting withSizeCheck:NO];
    else if (self.selectedInActionSheetOptions == TMapOptions::EMapWithCarRouting)
      [self performAction:DownloaderActionDownloadAll withSizeCheck:NO];
  }
}

- (void)download
{
  if (self.selectedInActionSheetOptions == TMapOptions::EMap)
    [self performAction:DownloaderActionDownloadMap withSizeCheck:NO];
  else if (self.selectedInActionSheetOptions == TMapOptions::ECarRouting)
    [self performAction:DownloaderActionDownloadCarRouting withSizeCheck:NO];
  else if (self.selectedInActionSheetOptions == TMapOptions::EMapWithCarRouting)
    [self performAction:DownloaderActionDownloadAll withSizeCheck:NO];
}

- (BOOL)allButtonsInActionSheetAreAboutDownloading:(TStatus const)status
{
  return status == TStatus::ENotDownloaded;
}

#pragma mark - Virtual methods

- (void)performAction:(DownloaderAction)action withSizeCheck:(BOOL)check {}
- (NSString *)parentTitle { return nil; }
- (NSString *)selectedMapName { return nil; }
- (uint64_t)selectedMapSizeWithOptions:(TMapOptions)options { return 0; }
- (TStatus)selectedMapStatus { return TStatus::EUnknown; }
- (TMapOptions)selectedMapOptions { return TMapOptions::EMap; }

#pragma mark - Virtual table view methods

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return 0; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath { return nil; }

#pragma mark - Public methods for successors

#define MB (1024 * 1024)

- (NSString *)formattedMapSize:(uint64_t)size
{
  NSString * sizeString;
  if (size > MB)
    sizeString = [NSString stringWithFormat:@"%llu %@", (size + 512 * 1024) / MB, L(@"mb")];
  else
    sizeString = [NSString stringWithFormat:@"%llu %@", (size + 1023) / 1024, L(@"kb")];
  return [sizeString uppercaseString];
}

- (BOOL)canDownloadSelectedMap
{
  uint64_t const size = [self selectedMapSizeWithOptions:self.selectedInActionSheetOptions];
  NSString * name = [self selectedMapName];

  Platform::EConnectionType const connection = Platform::ConnectionStatus();
  MWMAlertViewController * alert = [[MWMAlertViewController alloc] initWithViewController:self];
  if (connection != Platform::EConnectionType::CONNECTION_NONE)
  {
    if (connection == Platform::EConnectionType::CONNECTION_WWAN && size > 50 * MB)
    {
      NSString * title = [NSString stringWithFormat:L(@"no_wifi_ask_cellular_download"), name];
      [alert presentNotWifiAlertWithName:title downloadBlock:^
       {
         if (self.selectedInActionSheetOptions == TMapOptions::EMap)
           [self performAction:DownloaderActionDownloadMap withSizeCheck:NO];
         else if (self.selectedInActionSheetOptions == TMapOptions::ECarRouting)
           [self performAction:DownloaderActionDownloadCarRouting withSizeCheck:NO];
         else if (self.selectedInActionSheetOptions == TMapOptions::EMapWithCarRouting)
           [self performAction:DownloaderActionDownloadAll withSizeCheck:NO];
       }];
    }
    else
      return YES;
  }
  else
  {
    MWMAlertViewController * alert = [[MWMAlertViewController alloc] initWithViewController:self];
    [alert presentNotConnectionAlert];
  }
  return NO;
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
    [self addButtonWithTitle:L(@"zoom_to_country") action:DownloaderActionZoomToCountry toActionSheet:actionSheet];

  if (status == TStatus::ENotDownloaded || status == TStatus::EOutOfMemFailed || status == TStatus::EDownloadFailed)
  {
    NSString * size = [self formattedMapSize:[self selectedMapSizeWithOptions:TMapOptions::EMap]];
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

  if (status == TStatus::EOnDisk && options == TMapOptions::EMap)
  {
    NSString * size = [self formattedMapSize:[self selectedMapSizeWithOptions:TMapOptions::ECarRouting]];
    NSString * title = [NSString stringWithFormat:@"%@, %@", L(@"downloader_download_routing"), size];
    [self addButtonWithTitle:title action:DownloaderActionDownloadCarRouting toActionSheet:actionSheet];
  }

  if (status == TStatus::EOnDiskOutOfDate && options == TMapOptions::EMap)
  {
    NSString * size = [self formattedMapSize:[self selectedMapSizeWithOptions:TMapOptions::EMap]];
    NSString * title = [NSString stringWithFormat:@"%@, %@", L(@"downloader_update_map"), size];
    [self addButtonWithTitle:title action:DownloaderActionDownloadMap toActionSheet:actionSheet];
    size = [self formattedMapSize:[self selectedMapSizeWithOptions:TMapOptions::EMapWithCarRouting]];
    title = [NSString stringWithFormat:@"%@, %@", L(@"downloader_update_map_and_routing"), size];
    [self addButtonWithTitle:title action:DownloaderActionDownloadAll toActionSheet:actionSheet];
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
        self.selectedInActionSheetOptions = TMapOptions::EMap;
        break;

      case DownloaderActionDownloadCarRouting:
      case DownloaderActionDeleteCarRouting:
        self.selectedInActionSheetOptions = TMapOptions::ECarRouting;
        break;

      default:
        break;
    }

    [self performAction:action withSizeCheck:YES];
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
