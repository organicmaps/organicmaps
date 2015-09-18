
#import "ActiveMapsVC.h"
#import "Common.h"
#import "CountryTreeVC.h"
#import "Statistics.h"

extern NSString * const MapsStatusChangedNotification;

@interface CountryTreeVC () <CountryTreeObserverProtocol>

@end

@implementation CountryTreeVC
{
  CountryTree m_tree;
  CountryTreeObserver * m_treeObserver;
}

- (id)initWithNodePosition:(int)position
{
  self = [super init];

  if (position == -1)
  {
    self.tree.SetDefaultRoot();
    self.title = L(@"download_maps");
  }
  else
  {
    ASSERT(position < self.tree.GetChildCount(), ());
    self.title = @(self.tree.GetChildName(position).c_str());
    self.tree.SetChildAsRoot(position);
  }

  __weak CountryTreeVC * weakSelf = self;
  m_treeObserver = new CountryTreeObserver(weakSelf);
  self.tree.SetListener(m_treeObserver);

  return self;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(outOfDateCountriesCountChanged:) name:MapsStatusChangedNotification object:nil];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  m_tree = CountryTree();
  [self.tableView reloadData];
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  if (self.isMovingFromParentViewController)
  {
    if (self.tree.HasParent())
      self.tree.SetParentAsRoot();
    else
      self.tree.ResetListener();
  }
}

#define TOP_ROWS_COUNT 1

- (NSIndexPath *)downloadedCountriesIndexPath
{
  return [NSIndexPath indexPathForRow:0 inSection:0];
}

- (void)outOfDateCountriesCountChanged:(NSNotification *)notification
{
  NSInteger const count = [[notification userInfo][@"OutOfDate"] integerValue];
  if ([self activeMapsRowIsVisible])
  {
    MapCell * cell = (MapCell *)[self.tableView cellForRowAtIndexPath:[self downloadedCountriesIndexPath]];
    cell.badgeView.value = count;
  }
}

#pragma mark - Helpers

- (CountryTree &)tree
{
  if (m_tree.IsValid())
    return m_tree;
  else
    return GetFramework().GetCountryTree();
}

- (MapCell *)cellAtPositionInNode:(int)position
{
  NSInteger const section = [self activeMapsRowIsVisible] ? [self downloadedCountriesIndexPath].section + 1 : 0;
  return (MapCell *)[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:position inSection:section]];
}

- (BOOL)activeMapsRowIsVisible
{
  return !self.tree.GetActiveMapLayout().IsEmpty() && !self.tree.HasParent();
}

- (BOOL)isActiveMapsIndexPath:(NSIndexPath *)indexPath
{
  return [self activeMapsRowIsVisible] && [indexPath isEqual:[self downloadedCountriesIndexPath]];
}

- (void)configureSizeLabelOfMapCell:(MapCell *)cell position:(int)position status:(TStatus const &)status options:(MapOptions const &)options
{
  if (self.tree.IsLeaf(position))
  {
    if (status == TStatus::ENotDownloaded)
    {
      LocalAndRemoteSizeT const size = self.tree.GetRemoteLeafSizes(position);
      cell.sizeLabel.text = [NSString stringWithFormat:@"%@ / %@", formattedSize(size.first), formattedSize(size.second)];
    }
    else if (status == TStatus::EOnDisk || status == TStatus::EOnDiskOutOfDate)
      cell.sizeLabel.text = formattedSize(self.tree.GetLeafSize(position, options).second);
    else if (status == TStatus::EOutOfMemFailed || status == TStatus::EDownloadFailed || status == TStatus::EDownloading || status == TStatus::EInQueue)
      cell.sizeLabel.text = formattedSize(self.tree.GetDownloadableLeafSize(position).second);
  }
}

#pragma mark - DownloaderParentVC virtual methods implementation

- (NSString *)parentTitle
{
  return self.tree.IsCountryRoot() ? @(self.tree.GetRootName().c_str()) : nil;
}

- (NSString *)selectedMapName
{
  return @(self.tree.GetChildName(self.selectedPosition).c_str());
}

- (uint64_t)selectedMapSizeWithOptions:(MapOptions)options
{
  return self.tree.GetLeafSize(self.selectedPosition, options).second;
}

- (TStatus)selectedMapStatus
{
  return self.tree.GetLeafStatus(self.selectedPosition);
}

- (MapOptions)selectedMapOptions
{
  return self.tree.GetLeafOptions(self.selectedPosition);
}

- (void)performAction:(DownloaderAction)action withSizeCheck:(BOOL)check
{
  switch (action)
  {
    case DownloaderActionDownloadAll:
    case DownloaderActionDownloadMap:
    case DownloaderActionDownloadCarRouting:
      if (check == NO || [self canDownloadSelectedMap])
        self.tree.DownloadCountry(self.selectedPosition, self.selectedInActionSheetOptions);
      break;

    case DownloaderActionDeleteAll:
    case DownloaderActionDeleteMap:
    case DownloaderActionDeleteCarRouting:
      self.tree.DeleteCountry(self.selectedPosition, self.selectedInActionSheetOptions);
      break;

    case DownloaderActionCancelDownloading:
      self.tree.CancelDownloading(self.selectedPosition);
    break;

    case DownloaderActionZoomToCountry:
      self.tree.ShowLeafOnMap(self.selectedPosition);
      [[Statistics instance] logEvent:@"Show Map From Download Countries Screen"];
      [self.navigationController popToRootViewControllerAnimated:YES];
      break;
  }
}

#pragma mark - TableView

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  return 13;
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
  return 0.001;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return [self activeMapsRowIsVisible] ? 1 + TOP_ROWS_COUNT : 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if ([self activeMapsRowIsVisible] && section == [self downloadedCountriesIndexPath].section)
    return TOP_ROWS_COUNT;
  else
    return self.tree.GetChildCount();
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MapCell * cell = [tableView dequeueReusableCellWithIdentifier:[MapCell className]];
  if (!cell)
    cell = [[MapCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[MapCell className]];

  if ([self isActiveMapsIndexPath:indexPath])
  {
    cell.titleLabel.text = L(@"downloader_downloaded_maps");
    cell.subtitleLabel.text = nil;
    cell.parentMode = YES;
    cell.badgeView.value = self.tree.GetActiveMapLayout().GetOutOfDateCount();
    cell.separatorTop.hidden = NO;
    cell.separatorBottom.hidden = NO;
    cell.separator.hidden = YES;
  }
  else
  {
    int const position = static_cast<int>(indexPath.row);
    bool const isLeaf = self.tree.IsLeaf(position);
    NSInteger const numberOfRows = [self tableView:tableView numberOfRowsInSection:indexPath.section];
    BOOL const isLast = (indexPath.row == numberOfRows - 1);
    BOOL const isFirst = (indexPath.row == 0);

    cell.titleLabel.text = @(self.tree.GetChildName(position).c_str());
    cell.subtitleLabel.text = [self parentTitle];
    cell.delegate = self;
    cell.badgeView.value = 0;
    cell.parentMode = !isLeaf;
    cell.separatorTop.hidden = !isFirst;
    cell.separatorBottom.hidden = !isLast;
    cell.separator.hidden = isLast;
    if (isLeaf)
    {
      MapOptions const options = self.tree.GetLeafOptions(position);
      TStatus const status = self.tree.GetLeafStatus(position);
      if (status == TStatus::EOutOfMemFailed || status == TStatus::EDownloadFailed || status == TStatus::EDownloading || status == TStatus::EInQueue)
      {
        LocalAndRemoteSizeT const size = self.tree.GetDownloadableLeafSize(position);
        cell.downloadProgress = (double)size.first / size.second;
      }
      cell.status = status;
      cell.options = options;
      [self configureSizeLabelOfMapCell:cell position:position status:status options:options];
    }
  }

  return cell;
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
  int const position = static_cast<int>(indexPath.row);
  if (self.tree.IsLeaf(position))
  {
    TStatus const status = self.tree.GetLeafStatus(position);
    return status == TStatus::EOnDisk || status == TStatus::EOnDiskOutOfDate;
  }
  return NO;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (editingStyle == UITableViewCellEditingStyleDelete)
  {
    int const position = static_cast<int>(indexPath.row);
    self.tree.DeleteCountry(position, self.tree.GetLeafOptions(position));
    [tableView setEditing:NO animated:YES];
  }
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  if ([self isActiveMapsIndexPath:indexPath])
  {
    ActiveMapsVC * vc = [[ActiveMapsVC alloc] init];
    [self.navigationController pushViewController:vc animated:YES];
  }
  else
  {
    self.selectedPosition = static_cast<int>(indexPath.row);
    if (self.tree.IsLeaf(self.selectedPosition))
    {
      MapCell * cell = [self cellAtPositionInNode:self.selectedPosition];
      UIActionSheet * actionSheet = [self actionSheetToPerformActionOnSelectedMap];
      [actionSheet showFromRect:cell.frame inView:cell.superview animated:YES];
    }
    else
    {
      m_tree = self.tree;
      CountryTreeVC * vc = [[CountryTreeVC alloc] initWithNodePosition:self.selectedPosition];
      [self.navigationController pushViewController:vc animated:YES];
    }
  }
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  return [MapCell cellHeight];
}

#pragma mark - MapCellDelegate

- (void)mapCellDidStartDownloading:(MapCell *)cell
{
  self.selectedPosition = static_cast<int>([self.tableView indexPathForCell:cell].row);
  TStatus const status = [self selectedMapStatus];
  if (status == TStatus::EDownloadFailed || status == TStatus::EOutOfMemFailed)
    if ([self canDownloadSelectedMap])
      self.tree.RetryDownloading(self.selectedPosition);
}

- (void)mapCellDidCancelDownloading:(MapCell *)cell
{
  self.selectedPosition = static_cast<int>([self.tableView indexPathForCell:cell].row);
  [[self actionSheetToCancelDownloadingSelectedMap] showFromRect:cell.frame inView:cell.superview animated:YES];
}

#pragma mark - CountryTree core callbacks

- (void)countryStatusChangedAtPositionInNode:(int)position
{
  if ([self activeMapsRowIsVisible])
  {
    [self.tableView reloadData];
  }
  else
  {
    MapCell * cell = [self cellAtPositionInNode:position];
    TStatus const status = self.tree.GetLeafStatus(position);
    MapOptions const options = self.tree.GetLeafOptions(position);
    [self configureSizeLabelOfMapCell:cell position:position status:status options:options];
    [cell setStatus:self.tree.GetLeafStatus(position) options:options animated:YES];
  }
}

- (void)countryDownloadingProgressChanged:(LocalAndRemoteSizeT const &)progress atPositionInNode:(int)position
{
  MapCell * cell = [self cellAtPositionInNode:position];
  [cell setDownloadProgress:((double)progress.first / progress.second) animated:YES];
}

@end
