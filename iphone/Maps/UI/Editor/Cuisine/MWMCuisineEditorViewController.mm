#import "MWMCuisineEditorViewController.h"
#import "MWMKeyboard.h"
#import "MWMTableViewCell.h"
#import "SwiftBridge.h"

#include "indexer/cuisines.hpp"
#include "indexer/search_string_utils.hpp"

namespace
{
NSString * const kCuisineEditorCell = @"MWMCuisineEditorTableViewCell";
/// @returns pair.first in a separate vector.
std::vector<std::string> SliceKeys(std::vector<std::pair<std::string, std::string>> const & v)
{
  std::vector<std::string> res;
  for (auto const & kv : v)
    res.push_back(kv.first);
  return res;
}
}  // namespace

@interface MWMCuisineEditorViewController () <UISearchBarDelegate, MWMKeyboardObserver>
{
  osm::AllCuisines m_allCuisines;
  std::vector<std::string> m_selectedCuisines;
  std::vector<std::string> m_displayedKeys;
  std::vector<std::string> m_untranslatedKeys;
}

@property(weak, nonatomic) IBOutlet UITableView * tableView;
@property(weak, nonatomic) IBOutlet UISearchBar * searchBar;
@property(nonatomic) BOOL isSearch;

@end

@implementation MWMCuisineEditorViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configNavBar];
  [self configSearchBar];
  [self configData];
  [self configTable];
  [MWMKeyboard addObserver:self];
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
  return UIStatusBarStyleLightContent;
}

#pragma mark - MWMKeyboard

- (void)onKeyboardAnimation
{
  UIEdgeInsets const contentInsets = {.bottom = [MWMKeyboard keyboardHeight]};
  self.tableView.contentInset = contentInsets;
  self.tableView.scrollIndicatorInsets = contentInsets;
}

#pragma mark - UISearchBarDelegate

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText
{
  m_displayedKeys.clear();
  if (searchText.length)
  {
    self.isSearch = YES;
    std::string const st = searchText.UTF8String;
    for (auto const & kv : m_allCuisines)
      if (search::ContainsNormalized(kv.second, st))
        m_displayedKeys.push_back(kv.first);
  }
  else
  {
    self.isSearch = NO;
    m_displayedKeys = SliceKeys(m_allCuisines);
  }
  [self.tableView reloadData];
}

- (BOOL)searchBarShouldBeginEditing:(UISearchBar *)searchBar
{
  self.isSearch = NO;
  [self searchBar:searchBar setActiveState:YES];
  return YES;
}

- (BOOL)searchBarShouldEndEditing:(UISearchBar *)searchBar
{
  if (!searchBar.text.length)
  {
    self.isSearch = NO;
    [self searchBar:searchBar setActiveState:NO];
  }
  return YES;
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar
{
  self.isSearch = NO;
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
  self.isSearch = NO;
  self.searchBar.placeholder = L(@"search");
}

- (void)configData
{
  using namespace osm;
  m_allCuisines = Cuisines::Instance().AllSupportedCuisines();
  m_displayedKeys = SliceKeys(m_allCuisines);
  m_selectedCuisines = [self.delegate selectedCuisines];
  for (auto const & s : m_selectedCuisines)
  {
    std::string const translated = Cuisines::Instance().Translate(s);
    if (translated.empty())
      m_untranslatedKeys.push_back(s);
  }
}

- (void)configTable
{
  [self.tableView registerClass:[MWMTableViewCell class] forCellReuseIdentifier:[UITableViewCell className]];
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

- (void)change:(std::string const &)key selected:(BOOL)selected
{
  if (selected)
    m_selectedCuisines.push_back(key);
  else
    m_selectedCuisines.erase(std::find(m_selectedCuisines.begin(), m_selectedCuisines.end(), key));
}

#pragma mark - UITableViewDataSource

- (UITableViewCell * _Nonnull)tableView:(UITableView * _Nonnull)tableView
                  cellForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  auto cell = [tableView dequeueReusableCellWithCellClass:[UITableViewCell class] indexPath:indexPath];
  NSInteger const index = indexPath.row;

  auto const & dataSource = [self dataSourceForSection:indexPath.section];
  std::string const & key = dataSource[index];
  if (dataSource == m_displayedKeys)
  {
    std::string const translated = osm::Cuisines::Instance().Translate(m_displayedKeys[index]);
    NSAssert(!translated.empty(), @"There are only localizable keys in m_displayedKeys!");
    cell.textLabel.text = @(translated.c_str());
  }
  else
  {
    cell.textLabel.text = @(key.c_str());
  }

  BOOL const selected =
      std::find(m_selectedCuisines.begin(), m_selectedCuisines.end(), key) != m_selectedCuisines.end();
  cell.accessoryType = selected ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
  return cell;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return self.isSearch ? 1 : !m_untranslatedKeys.empty() + !m_displayedKeys.empty();
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self dataSourceForSection:section].size();
}

- (std::vector<std::string> const &)dataSourceForSection:(NSInteger)section
{
  if (m_untranslatedKeys.empty())
    return m_displayedKeys;
  else
    return self.isSearch ? m_displayedKeys : (section == 0 ? m_untranslatedKeys : m_displayedKeys);
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView * _Nonnull)tableView didSelectRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  UITableViewCell * cell = [tableView cellForRowAtIndexPath:indexPath];
  [cell setSelected:NO animated:YES];
  BOOL const isAlreadySelected = cell.accessoryType == UITableViewCellAccessoryCheckmark;
  cell.accessoryType = isAlreadySelected ? UITableViewCellAccessoryNone : UITableViewCellAccessoryCheckmark;
  [self change:[self dataSourceForSection:indexPath.section][indexPath.row] selected:!isAlreadySelected];
}

@end
