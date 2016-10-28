#import "Common.h"
#import "MWMAlertViewController.h"
#import "MWMCircularProgress.h"
#import "MWMDownloaderDialogCell.h"
#import "MWMDownloaderDialogHeader.h"
#import "MWMDownloadTransitMapAlert.h"
#import "MWMFrameworkListener.h"
#import "MWMStorage.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"
#import "UILabel+RuntimeAttributes.h"

#include "Framework.h"

namespace
{
NSString * const kCellIdentifier = @"MWMDownloaderDialogCell";
NSString * const kDownloadTransitMapAlertNibName = @"MWMDownloadTransitMapAlert";
NSString * const kStatisticsEvent = @"Map download Alert";

CGFloat const kCellHeight = 32.;
CGFloat const kHeaderHeight = 44.;
CGFloat const kMinimumOffset = 20.;
CGFloat const kAnimationDuration = .05;
} // namespace

@interface MWMDownloadTransitMapAlert () <UITableViewDataSource, UITableViewDelegate, MWMFrameworkStorageObserver, MWMCircularProgressProtocol>

@property (copy, nonatomic) TMWMVoidBlock cancelBlock;
@property (copy, nonatomic) TMWMDownloadBlock downloadBlock;
@property (copy, nonatomic) TMWMVoidBlock downloadCompleteBlock;

@property (nonatomic) MWMCircularProgress * progress;

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet UILabel * messageLabel;
@property (weak, nonatomic) IBOutlet UITableView * dialogsTableView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * tableViewHeight;
@property (nonatomic) MWMDownloaderDialogHeader * listHeader;
@property (nonatomic) BOOL listExpanded;

@property (weak, nonatomic) IBOutlet UIView * progressWrapper;
@property (weak, nonatomic) IBOutlet UIView * hDivider;
@property (weak, nonatomic) IBOutlet UIView * vDivider;
@property (weak, nonatomic) IBOutlet UIButton * leftButton;
@property (weak, nonatomic) IBOutlet UIButton * rightButton;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * dialogsBottomOffset;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * progressWrapperBottomOffset;

@property (copy, nonatomic) NSArray<NSString *> * countriesNames;
@property (copy, nonatomic) NSString * countriesSize;

@end

@implementation MWMDownloadTransitMapAlert
{
  storage::TCountriesVec m_countries;
}

+ (instancetype)downloaderAlertWithMaps:(storage::TCountriesVec const &)countries
                                   code:(routing::IRouter::ResultCode)code
                            cancelBlock:(TMWMVoidBlock)cancelBlock
                          downloadBlock:(TMWMDownloadBlock)downloadBlock
                  downloadCompleteBlock:(TMWMVoidBlock)downloadCompleteBlock
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatOpen}];
  MWMDownloadTransitMapAlert * alert = [self alertWithCountries:countries];
  switch (code)
  {
    case routing::IRouter::InconsistentMWMandRoute:
    case routing::IRouter::RouteNotFound:
    case routing::IRouter::RouteFileNotExist:
      alert.titleLabel.localizedText = @"dialog_routing_download_files";
      alert.messageLabel.localizedText = @"dialog_routing_download_and_update_all";
      break;
    case routing::IRouter::FileTooOld:
      alert.titleLabel.localizedText = @"dialog_routing_download_files";
      alert.messageLabel.localizedText = @"dialog_routing_download_and_update_maps";
      break;
    case routing::IRouter::NeedMoreMaps:
      alert.titleLabel.localizedText = @"dialog_routing_download_and_build_cross_route";
      alert.messageLabel.localizedText = @"dialog_routing_download_cross_route";
      break;
    default:
      NSAssert(false, @"Incorrect code!");
      break;
  }
  alert.cancelBlock = cancelBlock;
  alert.downloadBlock = downloadBlock;
  alert.downloadCompleteBlock = downloadCompleteBlock;
  return alert;
}

+ (instancetype)alertWithCountries:(storage::TCountriesVec const &)countries
{
  NSAssert(!countries.empty(), @"countries can not be empty.");
  MWMDownloadTransitMapAlert * alert = [[[NSBundle mainBundle] loadNibNamed:kDownloadTransitMapAlertNibName owner:nil options:nil] firstObject];

  NSMutableArray<NSString *> * titles = [@[] mutableCopy];
  TMwmSize totalSize = 0;
  auto const & s = GetFramework().GetStorage();
  for (auto const & countryId : countries)
  {
    storage::NodeAttrs attrs;
    s.GetNodeAttrs(countryId, attrs);
    [titles addObject:@(attrs.m_nodeLocalName.c_str())];
    totalSize += attrs.m_mwmSize;
  }

  alert->m_countries = countries;
  alert.countriesNames = titles;
  alert.countriesSize = formattedSize(totalSize);
  [alert configure];
  return alert;
}

- (void)configure
{
  [self.dialogsTableView registerNib:[UINib nibWithNibName:kCellIdentifier bundle:nil] forCellReuseIdentifier:kCellIdentifier];
  self.listExpanded = NO;
  [self.dialogsTableView reloadData];
}

#pragma mark - MWMCircularProgressProtocol

- (void)progressButtonPressed:(nonnull MWMCircularProgress *)progress
{
  for (auto const & countryId : m_countries)
    [MWMStorage cancelDownloadNode:countryId];
  [self cancelButtonTap];
}

#pragma mark - MWMFrameworkStorageObserver

- (void)processCountryEvent:(TCountryId const &)countryId
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
  if (overallProgress.first == overallProgress.second)
    [self close:self.downloadCompleteBlock];
}

- (void)processCountry:(TCountryId const &)countryId progress:(MapFilesDownloader::TProgress const &)progress
{
  auto const overallProgress = GetFramework().GetStorage().GetOverallProgress(m_countries);
  CGFloat const progressValue = static_cast<CGFloat>(overallProgress.first) / overallProgress.second;
  self.progress.progress = progressValue;
  self.titleLabel.text = [NSString stringWithFormat:@"%@%@%%", L(@"downloading"), @(floor(progressValue * 100))];
}

#pragma mark - Actions

- (IBAction)cancelButtonTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatClose}];
  [self close:self.cancelBlock];
}

- (IBAction)downloadButtonTap
{
  [Statistics logEvent:kStatisticsEvent withParameters:@{kStatAction : kStatApply}];
  self.downloadBlock(m_countries, ^
  {
    [MWMFrameworkListener addObserver:self];
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
  auto const updateCells = ^(BOOL show)
  {
    for (MWMDownloaderDialogCell * cell in self.dialogsTableView.visibleCells)
    {
      cell.titleLabel.alpha = show ? 1. : 0.;
    }
    [self.dialogsTableView beginUpdates];
    [self.dialogsTableView endUpdates];
  };
  if (listExpanded)
  {
    CGFloat const actualHeight = kCellHeight * m_countries.size() + kHeaderHeight;
    CGFloat const height = [self bounded:actualHeight withHeight:self.superview.height];
    self.tableViewHeight.constant = height;
    self.dialogsTableView.scrollEnabled = actualHeight > self.tableViewHeight.constant;
    [UIView animateWithDuration:kAnimationDuration animations:^{ [self layoutSubviews]; }
                     completion:^(BOOL finished)
    {
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
  CGFloat const height = UIInterfaceOrientationIsLandscape(orientation) ? MIN(self.superview.width, self.superview.height) : MAX(self.superview.width, self.superview.height);
  [self invalidateTableConstraintWithHeight:height];
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
  view.backgroundColor = UIColor.blackOpaque;
  return view;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMDownloaderDialogCell * cell = (MWMDownloaderDialogCell *)[tableView dequeueReusableCellWithIdentifier:kCellIdentifier];
  cell.titleLabel.text = self.countriesNames[indexPath.row];
  return cell;
}

#pragma mark - Properties

- (MWMDownloaderDialogHeader *)listHeader
{
  if (!_listHeader)
  {
    NSString * title = [NSString stringWithFormat:@"%@ (%@)", L(@"maps"), @(m_countries.size())];
    NSString * size = self.countriesSize;
    _listHeader = [MWMDownloaderDialogHeader headerForOwnerAlert:self title:title size:size];
  }
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
