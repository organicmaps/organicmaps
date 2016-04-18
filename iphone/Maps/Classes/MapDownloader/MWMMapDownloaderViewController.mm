#import "Common.h"
#import "MWMMapDownloaderExtendedDataSource.h"
#import "MWMMapDownloaderSearchDataSource.h"
#import "MWMMapDownloaderViewController.h"
#import "MWMNoMapsViewController.h"
#import "UIColor+MapsMeColor.h"
#import "UIKitCategories.h"

#include "Framework.h"

#include "storage/downloader_search_params.hpp"

namespace
{
NSString * const kNoMapsSegue = @"MapDownloaderEmbedNoMapsSegue";
} // namespace

using namespace storage;

@interface MWMBaseMapDownloaderViewController ()

@property (weak, nonatomic) IBOutlet UITableView * tableView;

@property (nonatomic) MWMMapDownloaderDataSource * dataSource;
@property (nonatomic) MWMMapDownloaderDataSource * defaultDataSource;

@property (nonatomic, readonly) NSString * parentCountryId;
@property (nonatomic, readonly) TMWMMapDownloaderMode mode;

- (void)configViews;

- (void)openAvailableMaps;

- (void)reloadTable;

@end

@interface MWMMapDownloaderViewController () <UISearchBarDelegate, UIScrollViewDelegate, MWMNoMapsViewControllerProtocol>

@property (weak, nonatomic) IBOutlet UIView * statusBarBackground;
@property (weak, nonatomic) IBOutlet UISearchBar * searchBar;
@property (weak, nonatomic) IBOutlet UIView * noMapsContainer;
@property (nonatomic) MWMNoMapsViewController * noMapsController;

@property (nonatomic) MWMMapDownloaderDataSource * searchDataSource;

@end

@implementation MWMMapDownloaderViewController
{
  DownloaderSearchParams m_searchParams;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.searchBar.placeholder = L(@"downloader_search_field_hint");
  [self setupSearchParams];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  UIColor * searchBarColor = [UIColor primary];
  self.statusBarBackground.backgroundColor = self.searchBar.barTintColor = searchBarColor;
  self.searchBar.backgroundImage = [UIImage imageWithColor:searchBarColor];
}

- (void)backTap
{
  NSArray<UIViewController *> * viewControllers = self.navigationController.viewControllers;
  UIViewController * previousViewController = viewControllers[viewControllers.count - 2];
  if ([previousViewController isKindOfClass:[MWMBaseMapDownloaderViewController class]])
    [self.navigationController popViewControllerAnimated:YES];
  else
    [self.navigationController popToRootViewControllerAnimated:YES];
}

- (void)configViews
{
  [super configViews];
  [self checkAndConfigNoMapsView];
}

#pragma mark - No Maps

- (void)checkAndConfigNoMapsView
{
  auto const & s = GetFramework().Storage();
  if (![self.parentCountryId isEqualToString:@(s.GetRootId().c_str())])
    return;
  if (self.mode == TMWMMapDownloaderMode::Available || s.HaveDownloadedCountries())
  {
    [self configAllMapsView];
    self.tableView.hidden = NO;
    self.noMapsContainer.hidden = YES;
  }
  else
  {
    self.showAllMapsView = NO;
    self.tableView.hidden = YES;
    self.noMapsContainer.hidden = NO;
  }
}

#pragma mark - All Maps Action

- (void)configAllMapsView
{
  self.showAllMapsView = NO;
  auto const & s = GetFramework().Storage();
  Storage::UpdateInfo updateInfo{};
  if (!s.GetUpdateInfo(s.GetRootId(), updateInfo) || updateInfo.m_numberOfMwmFilesToUpdate == 0)
    return;
  self.allMapsLabel.text =
      [NSString stringWithFormat:@"%@: %@ (%@)", L(@"downloader_outdated_maps"),
                                 @(updateInfo.m_numberOfMwmFilesToUpdate),
                                 formattedSize(updateInfo.m_totalUpdateSizeInBytes)];
  self.showAllMapsView = YES;
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
  if (searchText.length == 0)
  {
    self.dataSource = self.defaultDataSource;
    [self reloadTable];
  }
  else
  {
    NSString * primaryLanguage = self.searchBar.textInputMode.primaryLanguage;
    if (primaryLanguage)
      m_searchParams.m_inputLocale = primaryLanguage.UTF8String;

    m_searchParams.m_query = searchText.precomposedStringWithCompatibilityMapping.UTF8String;
    GetFramework().SearchInDownloader(m_searchParams);
  }
}

#pragma mark - UIBarPositioningDelegate

- (UIBarPosition)positionForBar:(id<UIBarPositioning>)bar
{
  return UIBarPositionTopAttached;
}

#pragma mark - Search

- (void)setupSearchParams
{
  __weak auto weakSelf = self;
  m_searchParams.m_onResults = ^(DownloaderSearchResults const & results)
  {
    __strong auto self = weakSelf;
    if (!self || results.m_endMarker)
      return;
    self.searchDataSource = [[MWMMapDownloaderSearchDataSource alloc] initWithSearchResults:results delegate:self];
    dispatch_async(dispatch_get_main_queue(), ^
    {
      self.dataSource = self.searchDataSource;
      [self reloadTable];
    });
  };
}

#pragma mark - MWMNoMapsViewControllerProtocol

- (void)handleDownloadMapsAction
{
  [self openAvailableMaps];
}

#pragma mark - Segue

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
  [super prepareForSegue:segue sender:sender];
  if ([segue.identifier isEqualToString:kNoMapsSegue])
  {
    self.noMapsController = segue.destinationViewController;
    self.noMapsController.delegate = self;
  }
}

#pragma mark - Configuration

- (void)setParentCountryId:(NSString *)parentId mode:(TMWMMapDownloaderMode)mode
{
  self.defaultDataSource = [[MWMMapDownloaderExtendedDataSource alloc] initForRootCountryId:parentId
                                                                                   delegate:self
                                                                                       mode:mode];
}

@end
