#import "Common.h"
#import "MWMMapDownloaderCountryTableViewCell.h"
#import "MWMMapDownloaderLargeCountryTableViewCell.h"
#import "MWMMapDownloaderPlaceTableViewCell.h"
#import "MWMMapDownloaderSubplaceTableViewCell.h"
#import "MWMMapDownloaderTableViewHeader.h"
#import "MWMMapDownloaderViewController.h"
#import "UIColor+MapsMeColor.h"
#import "UIKitCategories.h"

#include "std/vector.hpp"

static NSString * const kCountryCellIdentifier = @"MWMMapDownloaderCountryTableViewCell";
static NSString * const kLargeCountryCellIdentifier = @"MWMMapDownloaderLargeCountryTableViewCell";
static NSString * const kSubplaceCellIdentifier = @"MWMMapDownloaderSubplaceTableViewCell";
extern NSString * const kPlaceCellIdentifier;

static NSUInteger const sectionsCount = 2;
static NSUInteger const cellsCount = 4;
static NSString * const kCellType = @"kCellType";
static NSString * const kCellTitle = @"kCellTitle";
static NSString * const kCellDownloadSize = @"kCellDownloadSize";
static NSString * const kCellMapsCount = @"kCellMapsCount";
static NSString * const kCellArea = @"kCellArea";
static NSString * const kCellSubplace = @"kCellSubplace";

@interface MWMMapDownloaderViewController ()<UISearchBarDelegate, UIScrollViewDelegate>

@property (weak, nonatomic) IBOutlet UIView * statusBarBackground;
@property (weak, nonatomic) IBOutlet UISearchBar * searchBar;
@property (weak, nonatomic) IBOutlet UITableView * tableView;

@property (nonatomic) NSMutableDictionary * dataSource;

@end

@implementation MWMMapDownloaderViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"download_maps");
  self.searchBar.placeholder = L(@"search_downloaded_maps");

  self.dataSource = [NSMutableDictionary dictionary];
  self.dataSource[[NSIndexPath indexPathForRow:0 inSection:0]] = @{kCellType : kCountryCellIdentifier, kCellTitle : @"Algeria", kCellDownloadSize : @"123 MB"};
  self.dataSource[[NSIndexPath indexPathForRow:1 inSection:0]] = @{kCellType : kPlaceCellIdentifier, kCellTitle : @"London", kCellDownloadSize : @"456 MB", kCellArea : @"UK"};
  self.dataSource[[NSIndexPath indexPathForRow:2 inSection:0]] = @{kCellType : kLargeCountryCellIdentifier, kCellTitle : @"Brazil", kCellDownloadSize : @"789 MB", kCellMapsCount : @"14 maps"};
  self.dataSource[[NSIndexPath indexPathForRow:3 inSection:0]] = @{kCellType : kSubplaceCellIdentifier, kCellTitle : @"Western Cape", kCellDownloadSize : @"1234 MB", kCellArea : @"South Africa", kCellSubplace : @"Mossel Bay"};

  self.dataSource[[NSIndexPath indexPathForRow:0 inSection:1]] = @{kCellType : kCountryCellIdentifier, kCellTitle : @"Соединенное Королевство Великобритании и Северной Ирландии", kCellDownloadSize : @"9876 МБ"};
  self.dataSource[[NSIndexPath indexPathForRow:1 inSection:1]] = @{kCellType : kPlaceCellIdentifier, kCellTitle : @"กรุงเทพมหานคร อมรรัตนโกสินทร์ มหินทรายุธยามหาดิลก ภพนพรัตน์ ราชธานีบุรีรมย์ อุดมราชนิเวศน์ มหาสถาน อมรพิมาน อวตารสถิต สักกะทัตติยะ วิษณุกรรมประสิทธิ์", kCellDownloadSize : @"999 MB", kCellArea : @"Таиланд"};
  self.dataSource[[NSIndexPath indexPathForRow:2 inSection:1]] = @{kCellType : kLargeCountryCellIdentifier, kCellTitle : @"Muckanaghederdauhaulia", kCellDownloadSize : @"1234 MB", kCellMapsCount : @"999 maps"};
  self.dataSource[[NSIndexPath indexPathForRow:3 inSection:1]] = @{kCellType : kSubplaceCellIdentifier, kCellTitle : @"กรุงเทพมหานคร อมรรัตนโกสินทร์ มหินทรายุธยามหาดิลก ภพนพรัตน์ ราชธานีบุรีรมย์ อุดมราชนิเวศน์ มหาสถาน อมรพิมาน อวตารสถิต สักกะทัตติยะ วิษณุกรรมประสิทธิ์", kCellDownloadSize : @"999 MB", kCellArea : @"Соединенное Королевство Великобритании и Северной Ирландии", kCellSubplace : @"Venkatanarasimharajuvaripeta"};
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  UIColor * searchBarColor = [UIColor primary];
  self.statusBarBackground.backgroundColor = self.searchBar.barTintColor = searchBarColor;
  self.searchBar.backgroundImage = [UIImage imageWithColor:searchBarColor];
}

#pragma mark - Table

- (void)configTable
{
  [super configTable];
  [self.tableView registerNib:[UINib nibWithNibName:kCountryCellIdentifier bundle:nil] forCellReuseIdentifier:kCountryCellIdentifier];
  [self.tableView registerNib:[UINib nibWithNibName:kLargeCountryCellIdentifier bundle:nil] forCellReuseIdentifier:kLargeCountryCellIdentifier];
  [self.tableView registerNib:[UINib nibWithNibName:kSubplaceCellIdentifier bundle:nil] forCellReuseIdentifier:kSubplaceCellIdentifier];
}

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  NSIndexPath * path = [NSIndexPath indexPathForRow:[indexPath indexAtPosition:1] inSection:[indexPath indexAtPosition:0]];
  return self.dataSource[path][kCellType];
}

#pragma mark - All Maps Action

- (void)configAllMapsView
{
  [super configAllMapsView];
  self.allMapsLabel.text = @"5 Outdated Maps (108 MB)";
  self.showAllMapsView = YES;
}

- (IBAction)allMapsAction
{
}

#pragma mark - Fill cells with data

- (void)fillCell:(MWMMapDownloaderTableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSDictionary * data = self.dataSource[indexPath];
  BOOL const isLastCellInSection = indexPath.row == 3;
  if ([cell isKindOfClass:[MWMMapDownloaderCountryTableViewCell class]])
  {
    MWMMapDownloaderCountryTableViewCell * tCell = (MWMMapDownloaderCountryTableViewCell *)cell;
    tCell.title.text = data[kCellTitle];
    tCell.downloadSize.text = data[kCellDownloadSize];
    tCell.separator.hidden = isLastCellInSection;
  }
  else if ([cell isKindOfClass:[MWMMapDownloaderPlaceTableViewCell class]])
  {
    MWMMapDownloaderPlaceTableViewCell * tCell = (MWMMapDownloaderPlaceTableViewCell *)cell;
    tCell.title.text = data[kCellTitle];
    tCell.downloadSize.text = data[kCellDownloadSize];
    tCell.area.text = data[kCellArea];
    tCell.separator.hidden = isLastCellInSection;
  }
  else if ([cell isKindOfClass:[MWMMapDownloaderLargeCountryTableViewCell class]])
  {
    MWMMapDownloaderLargeCountryTableViewCell * tCell = (MWMMapDownloaderLargeCountryTableViewCell *)cell;
    tCell.title.text = data[kCellTitle];
    tCell.downloadSize.text = data[kCellDownloadSize];
    tCell.mapsCount.text = data[kCellMapsCount];
    tCell.separator.hidden = isLastCellInSection;
  }
  else if ([cell isKindOfClass:[MWMMapDownloaderSubplaceTableViewCell class]])
  {
    MWMMapDownloaderSubplaceTableViewCell * tCell = (MWMMapDownloaderSubplaceTableViewCell *)cell;
    tCell.title.text = data[kCellTitle];
    tCell.downloadSize.text = data[kCellDownloadSize];
    tCell.area.text = data[kCellArea];
    tCell.subPlace.text = data[kCellSubplace];
    tCell.separator.hidden = isLastCellInSection;
  }
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView * _Nonnull)tableView
{
  return sectionsCount;
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  return cellsCount;
}

- (NSArray<NSString *> * _Nullable)sectionIndexTitlesForTableView:(UITableView * _Nonnull)tableView
{
//  return nil;
  return @[@"A", @"Z"];
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView sectionForSectionIndexTitle:(NSString * _Nonnull)title atIndex:(NSInteger)index
{
  return index;
}

#pragma mark - UITableViewDelegate

- (UIView * _Nullable)tableView:(UITableView * _Nonnull)tableView viewForHeaderInSection:(NSInteger)section
{
  MWMMapDownloaderTableViewHeader * headerView =
      [[[NSBundle mainBundle] loadNibNamed:@"MWMMapDownloaderTableViewHeader"
                                     owner:nil
                                   options:nil] firstObject];
  headerView.lastSection = (section == sectionsCount - 1);
  headerView.title.text = [NSString stringWithFormat:@"Header: %@", @(section)];
  return headerView;
}

- (CGFloat)tableView:(UITableView * _Nonnull)tableView heightForHeaderInSection:(NSInteger)section
{
  return [MWMMapDownloaderTableViewHeader height];
}

- (void)tableView:(UITableView * _Nonnull)tableView didSelectRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSString * identifier = [self cellIdentifierForIndexPath:indexPath];
  if ([identifier isEqualToString:kLargeCountryCellIdentifier])
  {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    [self performSegueWithIdentifier:@"MapDownloader2CountryDownloaderSegue" sender:indexPath];
  }
  else
  {
    [super tableView:tableView didSelectRowAtIndexPath:indexPath];
  }
}

#pragma mark - Segue

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(NSIndexPath *)indexPath
{
  MWMMapCountryDownloaderViewController * destVC = segue.destinationViewController;
  destVC.title = @"BRAZILIA";
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
