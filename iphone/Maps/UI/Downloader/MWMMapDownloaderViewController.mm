#import "MWMMapDownloaderViewController.h"
#import "MWMMapDownloaderExtendedDataSourceWithAds.h"
#import "MWMMapDownloaderSearchDataSource.h"
#import "SwiftBridge.h"

#include "Framework.h"

namespace
{
NSString * const kMapDownloaderNoResultsEmbedViewControllerSegue =
    @"MapDownloaderNoResultsEmbedViewControllerSegue";
}  // namespace

using namespace storage;

@interface MWMBaseMapDownloaderViewController ()

@property(weak, nonatomic) IBOutlet UITableView * tableView;

@property(nonatomic) MWMMapDownloaderDataSource * dataSource;
@property(nonatomic) MWMMapDownloaderDataSource * defaultDataSource;

@property(nonatomic, readonly) NSString * parentCountryId;
@property(nonatomic, readonly) MWMMapDownloaderMode mode;

@property(nonatomic) BOOL showAllMapsButtons;

- (void)configViews;

- (void)openAvailableMaps;

- (void)reloadTable;

@end

@interface MWMMapDownloaderViewController ()<UISearchBarDelegate, UIScrollViewDelegate>

@property(weak, nonatomic) IBOutlet UIView * statusBarBackground;
@property(weak, nonatomic) IBOutlet UISearchBar * searchBar;
@property(weak, nonatomic) IBOutlet UIView * noMapsContainer;
@property(nonatomic) MWMDownloaderNoResultsEmbedViewController * noResultsEmbedViewController;

@property(nonatomic) MWMMapDownloaderSearchDataSource * searchDataSource;

@property(nonatomic) NSTimeInterval lastSearchTimestamp;

@end

@implementation MWMMapDownloaderViewController
{
  DownloaderSearchParams m_searchParams;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.searchBar.placeholder = L(@"downloader_search_field_hint");
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  UIColor * searchBarColor = [UIColor primary];
  self.statusBarBackground.backgroundColor = self.searchBar.barTintColor = searchBarColor;
  self.searchBar.backgroundImage = [UIImage imageWithColor:searchBarColor];
}

- (void)configViews
{
  [super configViews];
  [self checkAndConfigNoMapsView];
}

#pragma mark - No Maps

- (void)checkAndConfigNoMapsView
{
  auto const & s = GetFramework().GetStorage();
  if (![self.parentCountryId isEqualToString:@(s.GetRootId().c_str())])
    return;

  auto const showResults = ^{
    [self configAllMapsView];
    self.tableView.hidden = NO;
    self.noMapsContainer.hidden = YES;
  };
  auto const showNoResults = ^(MWMDownloaderNoResultsScreen screen) {
    self.showAllMapsButtons = NO;
    self.tableView.hidden = YES;
    self.noMapsContainer.hidden = NO;
    self.noResultsEmbedViewController.screen = screen;
  };

  BOOL const noResults =
      self.dataSource == self.searchDataSource && self.searchDataSource.isEmpty;
  BOOL const isModeAvailable = self.mode == MWMMapDownloaderModeAvailable;
  BOOL const haveActiveMaps = s.HaveDownloadedCountries() || s.IsDownloadInProgress();

  if (noResults)
    showNoResults(MWMDownloaderNoResultsScreenNoSearchResults);
  else if (isModeAvailable || haveActiveMaps)
    showResults();
  else
    showNoResults(MWMDownloaderNoResultsScreenNoMaps);
}

#pragma mark - UISearchBarDelegate

- (BOOL)searchBarShouldBeginEditing:(UISearchBar *)searchBar
{
  [self.searchBar setShowsCancelButton:YES animated:YES];
  [self.navigationController setNavigationBarHidden:YES animated:YES];
  self.tableView.contentInset = self.tableView.scrollIndicatorInsets = {};
  return YES;
}

- (BOOL)searchBarShouldEndEditing:(UISearchBar *)searchBar
{
  [self.searchBar setShowsCancelButton:NO animated:YES];
  [self.navigationController setNavigationBarHidden:NO animated:YES];
  self.tableView.contentInset = self.tableView.scrollIndicatorInsets = {};
  return YES;
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar
{
  self.searchBar.text = @"";
  [self.searchBar resignFirstResponder];
  self.dataSource = self.defaultDataSource;
  [self reloadTable];
}

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText
{
  NSString * primaryLanguage = self.searchBar.textInputMode.primaryLanguage;
  if (primaryLanguage)
    m_searchParams.m_inputLocale = primaryLanguage.UTF8String;

  m_searchParams.m_query = searchText.precomposedStringWithCompatibilityMapping.UTF8String;
  [self updateSearchCallback];
  GetFramework().SearchInDownloader(m_searchParams);
}

#pragma mark - UIBarPositioningDelegate

- (UIBarPosition)positionForBar:(id<UIBarPositioning>)bar { return UIBarPositionTopAttached; }
#pragma mark - Search

- (void)updateSearchCallback
{
  __weak auto weakSelf = self;
  NSTimeInterval const timestamp = [NSDate date].timeIntervalSince1970;
  self.lastSearchTimestamp = timestamp;
  m_searchParams.m_onResults = [weakSelf, timestamp](DownloaderSearchResults const & results) {
    __strong auto self = weakSelf;
    if (!self || timestamp != self.lastSearchTimestamp)
      return;
    if (results.m_query.empty())
    {
      self.dataSource = self.defaultDataSource;
    }
    else
    {
      self.searchDataSource =
          [[MWMMapDownloaderSearchDataSource alloc] initWithSearchResults:results delegate:self];
      self.dataSource = self.searchDataSource;
    }
    [self reloadTable];
  };
}

#pragma mark - UIScrollViewDelegate

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView
{
  [self.searchBar resignFirstResponder];
}

#pragma mark - MWMNoMapsViewControllerProtocol

- (void)handleDownloadMapsAction { [self openAvailableMaps]; }
#pragma mark - Segue

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
  [super prepareForSegue:segue sender:sender];
  if ([segue.identifier isEqualToString:kMapDownloaderNoResultsEmbedViewControllerSegue])
    self.noResultsEmbedViewController = segue.destinationViewController;
}

#pragma mark - Configuration

- (void)setParentCountryId:(NSString *)parentId mode:(MWMMapDownloaderMode)mode
{
  self.defaultDataSource =
      [[MWMMapDownloaderExtendedDataSourceWithAds alloc] initForRootCountryId:parentId
                                                                     delegate:self
                                                                         mode:mode];
}

#pragma mark - Helpers

- (void)reloadTable
{
  [super reloadTable];
  [self checkAndConfigNoMapsView];
}

@end
