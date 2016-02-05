#import "Common.h"
#import "MWMMapDownloaderViewController.h"
#import "UIColor+MapsMeColor.h"
#import "UIKitCategories.h"

@interface MWMBaseMapDownloaderViewController ()

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath;
- (void)fillCell:(MWMMapDownloaderTableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath;

@end

@interface MWMMapDownloaderViewController ()<UISearchBarDelegate, UIScrollViewDelegate>

@property (weak, nonatomic) IBOutlet UIView * statusBarBackground;
@property (weak, nonatomic) IBOutlet UISearchBar * searchBar;
@property (weak, nonatomic) IBOutlet UITableView * tableView;

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
  self.showAllMapsView = YES;
}

- (IBAction)allMapsAction
{
  // TODO (igrechuhin) Add implementation
}

#pragma mark - Fill cells with data

- (void)fillCell:(MWMMapDownloaderTableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  [super fillCell:cell atIndexPath:indexPath];
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView * _Nonnull)tableView
{
  // TODO (igrechuhin) Add implementation
  return [super numberOfSectionsInTableView:tableView];
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  // TODO (igrechuhin) Add implementation
  return [super tableView:tableView numberOfRowsInSection:section];
}

- (NSArray<NSString *> * _Nullable)sectionIndexTitlesForTableView:(UITableView * _Nonnull)tableView
{
  // TODO (igrechuhin) Add implementation
  return [super sectionIndexTitlesForTableView:tableView];
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView sectionForSectionIndexTitle:(NSString * _Nonnull)title atIndex:(NSInteger)index
{
  // TODO (igrechuhin) Add implementation
  return [super tableView:tableView sectionForSectionIndexTitle:title atIndex:index];
}

#pragma mark - UITableViewDelegate

- (UIView * _Nullable)tableView:(UITableView * _Nonnull)tableView viewForHeaderInSection:(NSInteger)section
{
  // TODO (igrechuhin) Add implementation
  return [super tableView:tableView viewForHeaderInSection:section];
}

- (CGFloat)tableView:(UITableView * _Nonnull)tableView heightForHeaderInSection:(NSInteger)section
{
  // TODO (igrechuhin) Add implementation
  return [super tableView:tableView heightForHeaderInSection:section];
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
