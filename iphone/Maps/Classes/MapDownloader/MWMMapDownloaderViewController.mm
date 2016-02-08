#import "Common.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMMapDownloaderViewController.h"
#import "UIColor+MapsMeColor.h"
#import "UIKitCategories.h"

#include "Framework.h"

using namespace storage;

@interface MWMBaseMapDownloaderViewController ()

@property (weak, nonatomic) IBOutlet UITableView * tableView;

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath;
- (void)fillCell:(MWMMapDownloaderTableViewCell *)cell forCountryId:(TCountryId const &)countryId;
- (void)configData;
- (TCountryId)countryIdForIndexPath:(NSIndexPath *)indexPath;

@end

@interface MWMMapDownloaderViewController ()<UISearchBarDelegate, UIScrollViewDelegate>

@property (weak, nonatomic) IBOutlet UIView * statusBarBackground;
@property (weak, nonatomic) IBOutlet UISearchBar * searchBar;

@property (nonatomic) NSArray<NSString *> * closestCoutryIds;
@property (nonatomic) NSArray<NSString *> * downloadedCoutryIds;
@property (nonatomic) NSInteger baseSectionShift;
@property (nonatomic) NSInteger closestCountriesSection;
@property (nonatomic) NSInteger downloadedCountriesSection;

@end

@implementation MWMMapDownloaderViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.searchBar.placeholder = L(@"search_downloaded_maps");
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  UIColor * searchBarColor = [UIColor primary];
  self.statusBarBackground.backgroundColor = self.searchBar.barTintColor = searchBarColor;
  self.searchBar.backgroundImage = [UIImage imageWithColor:searchBarColor];
}

#pragma mark - Data

- (void)configData
{
  [super configData];
  self.baseSectionShift = 0;
  [self configNearMe];
  [self configDownloaded];
}

- (void)configNearMe
{
  LocationManager * lm = MapsAppDelegate.theApp.m_locationManager;
  if (!lm.lastLocationIsValid)
    return;
  auto & countryInfoGetter = GetFramework().CountryInfoGetter();
  TCountriesVec closestCoutryIds;
  countryInfoGetter.GetRegionsCountryId(lm.lastLocation.mercator, closestCoutryIds);
  NSMutableArray<NSString *> * nsClosestCoutryIds = [@[] mutableCopy];
  for (auto const & countryId : closestCoutryIds)
    [nsClosestCoutryIds addObject:@(countryId.c_str())];
  self.closestCoutryIds = [nsClosestCoutryIds copy];
  if (self.closestCoutryIds.count != 0)
    self.closestCountriesSection = self.baseSectionShift++;
}

- (void)configDownloaded
{
  auto const & s = GetFramework().Storage();
  TCountriesVec downloadedCoutryIds;
  s.GetDownloadedChildren([self GetRootCountryId], downloadedCoutryIds);
  NSMutableArray<NSString *> * nsDownloadedCoutryIds = [@[] mutableCopy];
  for (auto const & countryId : downloadedCoutryIds)
    [nsDownloadedCoutryIds addObject:@(countryId.c_str())];
  self.downloadedCoutryIds = [nsDownloadedCoutryIds copy];
  if (self.downloadedCoutryIds.count != 0)
    self.downloadedCountriesSection = self.baseSectionShift++;
}

- (TCountryId)countryIdForIndexPath:(NSIndexPath *)indexPath
{
  NSInteger const section = indexPath.section;
  NSInteger const row = indexPath.row;
  if (section >= self.baseSectionShift)
    return [super countryIdForIndexPath:[NSIndexPath indexPathForRow:row inSection:section - self.baseSectionShift]];
  if (section == self.closestCountriesSection)
    return self.closestCoutryIds[row].UTF8String;
  if (section == self.downloadedCountriesSection)
    return self.downloadedCoutryIds[row].UTF8String;
  NSAssert(NO, @"Invalid section");
  return kInvalidCountryId;
}

#pragma mark - Table

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  return [super cellIdentifierForIndexPath:indexPath];
}

#pragma mark - All Maps Action

- (void)configAllMapsView
{
  [super configAllMapsView];
  // TODO (igrechuhin) Add implementation
  self.allMapsLabel.text = @"5 Outdated Maps (108 MB)";
  self.showAllMapsView = NO;
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView * _Nonnull)tableView
{
  NSInteger numberOfSections = [super numberOfSectionsInTableView:tableView];
  return numberOfSections + self.baseSectionShift;
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  if (section >= self.baseSectionShift)
    return [super tableView:tableView numberOfRowsInSection:section - self.baseSectionShift];
  if (section == self.closestCountriesSection)
    return self.closestCoutryIds.count;
  if (section == self.downloadedCountriesSection)
    return self.downloadedCoutryIds.count;
  NSAssert(NO, @"Invalid section");
  return 0;
}

- (NSArray<NSString *> * _Nullable)sectionIndexTitlesForTableView:(UITableView * _Nonnull)tableView
{
  return [super sectionIndexTitlesForTableView:tableView];
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView sectionForSectionIndexTitle:(NSString * _Nonnull)title atIndex:(NSInteger)index
{
  return [super tableView:tableView sectionForSectionIndexTitle:title atIndex:index] + self.baseSectionShift;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (section >= self.baseSectionShift)
    return [super tableView:tableView titleForHeaderInSection:section - self.baseSectionShift];
  if (section == self.closestCountriesSection)
    return L(@"search_mode_nearme");
  if (section == self.downloadedCountriesSection)
    return L(@"downloader_downloaded_maps");
  NSAssert(NO, @"Invalid section");
  return @"";
}

#pragma mark - UISearchBarDelegate

- (BOOL)searchBarShouldBeginEditing:(UISearchBar * _Nonnull)searchBar
{
  [self.searchBar setShowsCancelButton:YES animated:YES];
  [self.navigationController setNavigationBarHidden:YES animated:YES];
  self.tableView.contentInset = self.tableView.scrollIndicatorInsets = {};
  return YES;
}

- (BOOL)searchBarShouldEndEditing:(UISearchBar * _Nonnull)searchBar
{
  return YES;
}

- (void)searchBarCancelButtonClicked:(UISearchBar * _Nonnull)searchBar
{
  self.searchBar.text = @"";
  [self.searchBar resignFirstResponder];
  [self.searchBar setShowsCancelButton:NO animated:YES];
  [self.navigationController setNavigationBarHidden:NO animated:YES];
  self.tableView.contentInset = self.tableView.scrollIndicatorInsets = {};
}

@end
