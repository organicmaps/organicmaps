#import "Common.h"
#import "DownloaderParentVC.h"
#import "DiskFreeSpace.h"
#import "MWMAlertViewController.h"
#import "UIColor+MapsMeColor.h"

#include "platform/platform.hpp"

@implementation DownloaderParentVC

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self.view addSubview:self.tableView];
  self.tableView.backgroundColor = [UIColor pressBackground];
  self.tableView.separatorColor = [UIColor blackDividers];
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
    [self download];
}

- (void)download
{
  if (self.selectedInActionSheetOptions == MapOptions::MapWithCarRouting)
    [self performAction:DownloaderActionDownloadMap withSizeCheck:NO];
}

#pragma mark - Virtual methods

- (void)performAction:(DownloaderAction)action withSizeCheck:(BOOL)check {}
- (NSString *)parentTitle { return nil; }
- (NSString *)selectedMapName { return nil; }
- (uint64_t)selectedMapSizeWithOptions:(MapOptions)options { return 0; }
- (TStatus)selectedMapStatus { return TStatus::EUnknown; }
- (MapOptions)selectedMapOptions { return MapOptions::Map; }

#pragma mark - Virtual table view methods

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return 0; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath { return nil; }

#pragma mark - Public methods for successors

- (BOOL)canDownloadSelectedMap
{
  uint64_t const size = [self selectedMapSizeWithOptions:self.selectedInActionSheetOptions];
  NSString * name = [self selectedMapName];

  Platform::EConnectionType const connection = Platform::ConnectionStatus();
  MWMAlertViewController * alert = [[MWMAlertViewController alloc] initWithViewController:self];
  if (connection != Platform::EConnectionType::CONNECTION_NONE)
  {
    if (connection == Platform::EConnectionType::CONNECTION_WWAN && size > 50 * MB)
      [alert presentnoWiFiAlertWithName:name downloadBlock:^{[self download];}];
    else
      return YES;
  }
  else
  {
    [alert presentNoConnectionAlert];
  }
  return NO;
}

- (UIActionSheet *)actionSheetToPerformActionOnSelectedMap
{
  [self.actionSheetActions removeAllObjects];
  UIActionSheet * actionSheet = [[UIActionSheet alloc] initWithTitle:self.actionSheetTitle delegate:self cancelButtonTitle:nil destructiveButtonTitle:nil otherButtonTitles:nil];
  NSString * fullSize = formattedSize([self selectedMapSizeWithOptions:MapOptions::MapWithCarRouting]);
  switch ([self selectedMapStatus])
  {
    case TStatus::EOnDisk:
      [self addButtonWithTitle:L(@"zoom_to_country") action:DownloaderActionZoomToCountry toActionSheet:actionSheet];
      [self addButtonWithTitle:L(@"downloader_delete_map") action:DownloaderActionDeleteMap toActionSheet:actionSheet];
      actionSheet.destructiveButtonIndex = actionSheet.numberOfButtons - 1;
      break;
    case TStatus::EOnDiskOutOfDate:
      [self addButtonWithTitle:L(@"zoom_to_country") action:DownloaderActionZoomToCountry toActionSheet:actionSheet];
      [self addButtonWithTitle:[NSString stringWithFormat:@"%@, %@", L(@"downloader_update_map"), fullSize] action:DownloaderActionDownloadMap toActionSheet:actionSheet];
      [self addButtonWithTitle:L(@"downloader_delete_map") action:DownloaderActionDeleteMap toActionSheet:actionSheet];
      actionSheet.destructiveButtonIndex = actionSheet.numberOfButtons - 1;
      break;
    case TStatus::EUnknown:
      NSAssert(NO, @"Invalid status");
      break;
    default:
      break;
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
    self.selectedInActionSheetOptions = MapOptions::MapWithCarRouting;
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
