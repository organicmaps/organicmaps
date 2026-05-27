#import "MWMDownloadTransitMapAlert.h"
#import "MWMCircularProgress.h"
#import "MWMDownloaderDialogCell.h"
#import "MWMDownloaderDialogHeader.h"
#import "MWMStorage+UI.h"
#import "SwiftBridge.h"

#include <CoreApi/Framework.h>

#include "platform/downloader_defines.hpp"

namespace
{
NSString * const kDownloadTransitMapAlertNibName = @"MWMDownloadTransitMapAlert";

CGFloat const kCellHeight = 32.;
CGFloat const kHeaderHeight = 44.;
CGFloat const kMinimumOffset = 20.;
CGFloat const kAnimationDuration = .05;
}  // namespace

@interface MWMDownloadTransitMapAlert () <UITableViewDataSource,
                                          UITableViewDelegate,
                                          MWMStorageObserver,
                                          MWMCircularProgressProtocol>

@property(copy, nonatomic) MWMVoidBlock cancelBlock;
@property(copy, nonatomic) MWMDownloadBlock downloadBlock;
@property(copy, nonatomic) MWMVoidBlock downloadCompleteBlock;

@property(nonatomic) MWMCircularProgress * progress;

@property(weak, nonatomic) IBOutlet UIView * containerView;
@property(weak, nonatomic) IBOutlet UILabel * titleLabel;
@property(weak, nonatomic) IBOutlet UILabel * messageLabel;
@property(weak, nonatomic) IBOutlet UITableView * dialogsTableView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * tableViewHeight;
@property(nonatomic) MWMDownloaderDialogHeader * listHeader;
@property(nonatomic) BOOL listExpanded;

@property(weak, nonatomic) IBOutlet UIView * progressWrapper;
@property(weak, nonatomic) IBOutlet UIView * hDivider;
@property(weak, nonatomic) IBOutlet UIView * vDivider;
@property(weak, nonatomic) IBOutlet UIButton * leftButton;
@property(weak, nonatomic) IBOutlet UIButton * rightButton;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * dialogsBottomOffset;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * progressWrapperBottomOffset;

@property(copy, nonatomic) NSArray<NSString *> * countriesNames;
@property(copy, nonatomic) NSString * countriesSize;

@end

@implementation MWMDownloadTransitMapAlert
{
  storage::CountriesVec m_countries;
}

+ (instancetype)downloaderAlertWithMaps:(storage::CountriesSet const &)countries
                                   code:(routing::RouterResultCode)code
                            cancelBlock:(MWMVoidBlock)cancelBlock
                          downloadBlock:(MWMDownloadBlock)downloadBlock
                  downloadCompleteBlock:(MWMVoidBlock)downloadCompleteBlock
{
  MWMDownloadTransitMapAlert * alert = [self alertWithCountries:countries];
  switch (code)
  {
  case routing::RouterResultCode::InconsistentMWMandRoute:
  case routing::RouterResultCode::RouteNotFound:
  case routing::RouterResultCode::RouteFileNotExist:
    alert.titleLabel.text = L(@"dialog_routing_download_files");
    alert.messageLabel.text = L(@"dialog_routing_download_and_update_all");
    break;
  case routing::RouterResultCode::FileTooOld:
    alert.titleLabel.text = L(@"dialog_routing_download_files");
    alert.messageLabel.text = L(@"dialog_routing_download_and_update_maps");
    break;
  case routing::RouterResultCode::NeedMoreMaps:
    alert.titleLabel.text = L(@"dialog_routing_download_and_build_cross_route");
    alert.messageLabel.text = L(@"dialog_routing_download_cross_route");
    break;
  default: NSAssert(false, @"Incorrect code!"); break;
  }
  alert.cancelBlock = cancelBlock;
  alert.downloadBlock = downloadBlock;
  alert.downloadCompleteBlock = downloadCompleteBlock;
  return alert;
}

+ (instancetype)alertWithCountries:(storage::CountriesSet const &)countries
{
  NSAssert(!countries.empty(), @"countries can not be empty.");
  MWMDownloadTransitMapAlert * alert =
      [NSBundle.mainBundle loadNibNamed:kDownloadTransitMapAlertNibName owner:nil options:nil].firstObject;

  alert->m_countries = storage::CountriesVec(countries.begin(), countries.end());
  [alert configure];
  [alert updateCountriesList];
  [[MWMStorage sharedStorage] addObserver:alert];
  return alert;
}

- (void)configure
{
  [self.dialogsTableView registerNibWithCellClass:[MWMDownloaderDialogCell class]];
  self.listExpanded = NO;
  CALayer * containerViewLayer = self.containerView.layer;
  containerViewLayer.shouldRasterize = YES;
  containerViewLayer.rasterizationScale = [[UIScreen mainScreen] scale];
  [self.dialogsTableView reloadData];
}

- (void)updateCountriesList
{
  auto const & s = GetFramework().GetStorage();
  m_countries.erase(remove_if(m_countries.begin(), m_countries.end(),
                              [&s](storage::CountryId const & countryId) { return s.HasLatestVersion(countryId); }),
                    m_countries.end());
  NSMutableArray<NSString *> * titles = [@[] mutableCopy];
  MwmSize totalSize = 0;
  for (auto const & countryId : m_countries)
  {
    storage::NodeAttrs attrs;
    s.GetNodeAttrs(countryId, attrs);
    [titles addObject:@(attrs.m_nodeLocalName.c_str())];
    totalSize += attrs.m_mwmSize;
  }
  self.countriesNames = titles;
  self.countriesSize = formattedSize(totalSize);
}

#pragma mark - MWMCircularProgressProtocol

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress
{
  for (auto const & countryId : m_countries)
    [[MWMStorage sharedStorage] cancelDownloadNode:@(countryId.c_str())];
  [self cancelButtonTap];
}

#pragma mark - MWMStorageObserver

- (void)processCountryEvent:(NSString *)countryId
{
  if (find(m_countries.begin(), m_countries.end(), countryId.UTF8String) == m_countries.end())
    return;
  if (self.rightButton.hidden)
  {
    auto const & s = GetFramework().GetStorage();
    auto const & p = GetFramework().GetDownloadingPolicy();
    if (s.CheckFailedCountries(m_countries))
    {
      if (p.IsAutoRetryDownloadFailed())
        [self close:nil];
      return;
    }
    auto const overallProgress = s.GetOverallProgress(m_countries);
    // Test if downloading has finished by comparing downloaded and total sizes.
    if (overallProgress.m_bytesDownloaded == overallProgress.m_bytesTotal)
      [self close:self.downloadCompleteBlock];
  }
  else
  {
    [self updateCountriesList];
    [self.dialogsTableView reloadSections:[NSIndexSet indexSetWithIndex:0]
                         withRowAnimation:UITableViewRowAnimationAutomatic];
    if (m_countries.empty())
      [self close:self.downloadCompleteBlock];
  }
}

- (void)processCountry:(NSString *)countryId downloadedBytes:(uint64_t)downloadedBytes totalBytes:(uint64_t)totalBytes
{
  if (!self.rightButton.hidden ||
      find(m_countries.begin(), m_countries.end(), countryId.UTF8String) == m_countries.end())
    return;
  auto const overallProgress = GetFramework().GetStorage().GetOverallProgress(m_countries);
  CGFloat const progressValue = static_cast<CGFloat>(overallProgress.m_bytesDownloaded) / overallProgress.m_bytesTotal;
  self.progress.progress = progressValue;
  self.titleLabel.text = [NSString stringWithFormat:@"%@%@%%", L(@"downloading"), @(floor(progressValue * 100))];
}

#pragma mark - Actions

- (IBAction)cancelButtonTap
{
  [self close:self.cancelBlock];
}

- (IBAction)downloadButtonTap
{
  [self updateCountriesList];
  if (m_countries.empty())
  {
    [self close:self.downloadCompleteBlock];
    return;
  }
  self.downloadBlock(m_countries, ^{
    self.titleLabel.text = L(@"downloading");
    self.messageLabel.hidden = YES;
    self.progressWrapper.hidden = NO;
    self.progress.state = MWMCircularProgressStateSpinner;
    self.hDivider.hidden = YES;
    self.vDivider.hidden = YES;
    self.leftButton.hidden = YES;
    self.rightButton.hidden = YES;
    self.dialogsBottomOffset.priority = UILayoutPriorityDefaultHigh;
    self.progressWrapperBottomOffset.priority = UILayoutPriorityDefaultHigh;
    [UIView animateWithDuration:kAnimationDuration animations:^{ [self layoutSubviews]; }];
  });
}

- (void)showDownloadDetail:(UIButton *)sender
{
  self.listExpanded = sender.selected;
}

- (void)setListExpanded:(BOOL)listExpanded
{
  _listExpanded = listExpanded;
  [self layoutIfNeeded];
  auto const updateCells = ^(BOOL show) {
    for (MWMDownloaderDialogCell * cell in self.dialogsTableView.visibleCells)
      cell.titleLabel.alpha = show ? 1. : 0.;
    [self.dialogsTableView refresh];
  };
  if (listExpanded)
  {
    CGFloat const actualHeight = kCellHeight * m_countries.size() + kHeaderHeight;
    CGFloat const height = [self bounded:actualHeight withHeight:self.superview.height];
    self.tableViewHeight.constant = height;
    self.dialogsTableView.scrollEnabled = actualHeight > self.tableViewHeight.constant;
    [UIView animateWithDuration:kAnimationDuration
        animations:^{ [self layoutSubviews]; }
        completion:^(BOOL finished) {
          [UIView animateWithDuration:kDefaultAnimationDuration animations:^{ updateCells(YES); }];
        }];
  }
  else
  {
    self.tableViewHeight.constant = kHeaderHeight;
    self.dialogsTableView.scrollEnabled = NO;
    updateCells(NO);
    [UIView animateWithDuration:kAnimationDuration animations:^{ [self layoutSubviews]; }];
  }
}

- (CGFloat)bounded:(CGFloat)f withHeight:(CGFloat)h
{
  CGFloat const currentHeight = [self.subviews.firstObject height];
  CGFloat const maximumHeight = h - 2. * kMinimumOffset;
  CGFloat const availableHeight = maximumHeight - currentHeight;
  return MIN(f, availableHeight + self.tableViewHeight.constant);
}

- (void)invalidateTableConstraintWithHeight:(CGFloat)height
{
  if (self.listExpanded)
  {
    CGFloat const actualHeight = kCellHeight * m_countries.size() + kHeaderHeight;
    self.tableViewHeight.constant = [self bounded:actualHeight withHeight:height];
    self.dialogsTableView.scrollEnabled = actualHeight > self.tableViewHeight.constant;
  }
  else
  {
    self.tableViewHeight.constant = kHeaderHeight;
  }
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
{
  CGFloat const height = UIInterfaceOrientationIsLandscape(orientation)
                           ? MIN(self.superview.width, self.superview.height)
                           : MAX(self.superview.width, self.superview.height);
  [self invalidateTableConstraintWithHeight:height];
}

- (void)close:(MWMVoidBlock)completion
{
  [[MWMStorage sharedStorage] removeObserver:self];
  [super close:completion];
}

#pragma mark - UITableViewDelegate

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_countries.size();
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  return kHeaderHeight;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  return kCellHeight;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
  return self.listHeader;
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section
{
  UIView * view = [[UIView alloc] init];
  view.styleName = @"BlackOpaqueBackground";
  return view;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  Class cls = [MWMDownloaderDialogCell class];
  auto cell = static_cast<MWMDownloaderDialogCell *>([tableView dequeueReusableCellWithCellClass:cls
                                                                                       indexPath:indexPath]);
  cell.titleLabel.text = self.countriesNames[indexPath.row];
  return cell;
}

#pragma mark - Properties

- (MWMDownloaderDialogHeader *)listHeader
{
  if (!_listHeader)
    _listHeader = [MWMDownloaderDialogHeader headerForOwnerAlert:self];

  [_listHeader setTitle:[NSString stringWithFormat:@"%@ (%@)", L(@"downloader_status_maps"), @(m_countries.size())]
                   size:self.countriesSize];
  return _listHeader;
}

- (MWMCircularProgress *)progress
{
  if (!_progress)
  {
    _progress = [MWMCircularProgress downloaderProgressForParentView:self.progressWrapper];
    _progress.delegate = self;
  }
  return _progress;
}

@end
