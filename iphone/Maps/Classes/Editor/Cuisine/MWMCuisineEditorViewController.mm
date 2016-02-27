#import "MWMCuisineEditorTableViewCell.h"
#import "MWMCuisineEditorViewController.h"
#import "UIColor+MapsMeColor.h"

#include "indexer/search_string_utils.hpp"
#include "indexer/cuisines.hpp"

#include "std/algorithm.hpp"
#include "std/set.hpp"

namespace
{
NSString * const kCuisineEditorCell = @"MWMCuisineEditorTableViewCell";
/// @returns pair.first in a separate vector.
vector<string> SliceKeys(vector<pair<string, string>> const & v)
{
  vector<string> res;
  for (auto const & kv : v)
    res.push_back(kv.first);
  return res;
}
} // namespace

@interface MWMCuisineEditorViewController ()<MWMCuisineEditorTableViewCellProtocol, UISearchBarDelegate>
{
  osm::TAllCuisines m_allCuisines;
  vector<string> m_selectedCuisines;
  vector<string> m_displayedKeys;
}

@property (weak, nonatomic) IBOutlet UISearchBar * searchBar;

@end

@implementation MWMCuisineEditorViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configNavBar];
  [self configSearchBar];
  [self configData];
  [self configTable];
}

#pragma mark - UISearchBarDelegate

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText
{
  m_displayedKeys.clear();
  if (searchText.length)
  {
    string const st = searchText.UTF8String;
    for (auto const & kv : m_allCuisines)
      if (search::ContainsNormalized(kv.second, st))
        m_displayedKeys.push_back(kv.first);
  }
  else
  {
    m_displayedKeys = SliceKeys(m_allCuisines);
  }
  [self.tableView reloadData];
}

- (BOOL)searchBarShouldBeginEditing:(UISearchBar *)searchBar
{
  [self searchBar:searchBar setActiveState:YES];
  return YES;
}

- (BOOL)searchBarShouldEndEditing:(UISearchBar *)searchBar
{
  if (!searchBar.text.length)
    [self searchBar:searchBar setActiveState:NO];
  return YES;
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar
{
  [searchBar resignFirstResponder];
  searchBar.text = @"";
  [self searchBar:searchBar setActiveState:NO];
  m_displayedKeys = SliceKeys(m_allCuisines);
  [self.tableView reloadData];
}

- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar
{
  [searchBar resignFirstResponder];
}

- (UIBarPosition)positionForBar:(id<UIBarPositioning>)bar
{
  return UIBarPositionTopAttached;
}

- (void)searchBar:(UISearchBar *)searchBar setActiveState:(BOOL)isActiveState
{
  [searchBar setShowsCancelButton:isActiveState animated:YES];
  [self.navigationController setNavigationBarHidden:isActiveState animated:YES];
}

#pragma mark - Configuration

- (void)configNavBar
{
  self.title = L(@"cuisine");
  self.navigationItem.leftBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel
                                                    target:self
                                                    action:@selector(onCancel)];
  self.navigationItem.rightBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                                                    target:self
                                                    action:@selector(onDone)];
  self.navigationController.navigationBar.barStyle = UIBarStyleBlack;
}

- (void)configSearchBar
{
  self.searchBar.backgroundImage = [UIImage imageWithColor:[UIColor primary]];
  self.searchBar.placeholder = L(@"search_in_cuisine");
  UITextField * textFiled = [self.searchBar valueForKey:@"searchField"];
  UILabel * placeholder = [textFiled valueForKey:@"_placeholderLabel"];
  placeholder.textColor = [UIColor blackHintText];
}

- (void)configData
{
  m_allCuisines = osm::Cuisines::Instance().AllSupportedCuisines();
  m_displayedKeys = SliceKeys(m_allCuisines);
  m_selectedCuisines = [self.delegate getSelectedCuisines];
}

- (void)configTable
{
  [self.tableView registerNib:[UINib nibWithNibName:kCuisineEditorCell bundle:nil]
       forCellReuseIdentifier:kCuisineEditorCell];
}

#pragma mark - Actions

- (void)onCancel
{
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)onDone
{
  [self.delegate setSelectedCuisines:m_selectedCuisines];
  [self onCancel];
}

#pragma mark - MWMCuisineEditorTableViewCellProtocol

- (void)change:(string const &)key selected:(BOOL)selected
{
  if (selected)
    m_selectedCuisines.push_back(key);
  else
    m_selectedCuisines.erase(find(m_selectedCuisines.begin(), m_selectedCuisines.end(), key));
}

#pragma mark - UITableViewDataSource

- (UITableViewCell * _Nonnull)tableView:(UITableView * _Nonnull)tableView cellForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  return [tableView dequeueReusableCellWithIdentifier:kCuisineEditorCell];
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_displayedKeys.size();
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView * _Nonnull)tableView willDisplayCell:(MWMCuisineEditorTableViewCell * _Nonnull)cell forRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSInteger const index = indexPath.row;
  string const & key = m_displayedKeys[index];
  BOOL const selected = find(m_selectedCuisines.begin(), m_selectedCuisines.end(), key) != m_selectedCuisines.end();
  [cell configWithDelegate:self key:key translation:osm::Cuisines::Instance().Translate(key) selected:selected];
}

@end
