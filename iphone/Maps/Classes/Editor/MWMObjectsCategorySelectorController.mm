#import "MWMEditorViewController.h"
#import "MWMObjectsCategorySelectorController.h"
#import "UIColor+MapsMeColor.h"
#import "UIViewController+Navigation.h"

#include "Framework.h"

using namespace osm;

namespace
{
  NSString * const kToEditorSegue = @"CategorySelectorToEditorSegue";
} // namespace

@interface MWMObjectsCategorySelectorController () <UISearchBarDelegate>
{
  NewFeatureCategories m_categories;
  vector<Category> m_filtredCategories;
}

@property (weak, nonatomic) IBOutlet UISearchBar * searchBar;
@property (nonatomic) NSIndexPath * selectedIndexPath;
@property (nonatomic) BOOL isSearch;

@end

@implementation MWMObjectsCategorySelectorController

- (void)viewDidLoad
{
  [super viewDidLoad];
  if (m_categories.m_allSorted.size() == 0)
    m_categories = GetFramework().GetEditorCategories();
  self.isSearch = NO;
  [self configNavBar];
  [self configSearchBar];
}

- (void)setSelectedCategory:(const string &)category
{
  m_categories = GetFramework().GetEditorCategories();
  auto const & all = m_categories.m_allSorted;
  auto const it = find_if(all.begin(), all.end(), [&category](Category const & c)
  {
    return c.m_name == category;
  });
  NSAssert(it != all.end(), @"Incorrect category!");
  self.selectedIndexPath = [NSIndexPath indexPathForRow:(it - all.begin())
                                              inSection:m_categories.m_lastUsed.size() == 0 ? 0 : 1];
}

- (void)backTap
{
  auto const object = self.createdObject;
  //TODO(Vlad, Alex): Here we need to process incorrect (false) result of creating.
  [self.delegate reloadObject:object.second];
  [super backTap];
}

- (void)configNavBar
{
  self.title = L(@"feature_type").capitalizedString;
  if (!self.selectedIndexPath)
  {
    self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:L(@"done")
                                                  style:UIBarButtonItemStyleDone target:self action:@selector(onDone)];
  }
}

- (void)configSearchBar
{
  self.searchBar.backgroundImage = [UIImage imageWithColor:[UIColor primary]];
  self.searchBar.placeholder = L(@"search_in_types");
  UITextField * textFiled = [self.searchBar valueForKey:@"searchField"];
  UILabel * placeholder = [textFiled valueForKey:@"_placeholderLabel"];
  placeholder.textColor = [UIColor blackHintText];
}

- (void)onDone
{
  [self performSegueWithIdentifier:kToEditorSegue sender:nil];
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
  if (![segue.identifier isEqualToString:kToEditorSegue])
  {
    NSAssert(false, @"incorrect segue");
    return;
  }
  MWMEditorViewController * dest = static_cast<MWMEditorViewController *>(segue.destinationViewController);
  dest.isCreating = YES;
  auto const object = self.createdObject;
  //TODO(Vlad, Alex): Here we need to process incorrect (false) result of creating.
  [dest setEditableMapObject:object.second];
}

#pragma mark - Create object

- (pair<bool, EditableMapObject>)createdObject
{
  auto const & ds = [self dataSourceForSection:self.selectedIndexPath.section];
  EditableMapObject emo;
  bool const itsOK = GetFramework().CreateMapObjectAtViewportCenter(ds[self.selectedIndexPath.row].m_type, emo);
  return {itsOK, emo};
}

#pragma mark - UITableView

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:[UITableViewCell className]];
  cell.textLabel.textColor = [UIColor blackPrimaryText];
  cell.backgroundColor = [UIColor white];
  cell.textLabel.text = @([self dataSourceForSection:indexPath.section][indexPath.row].m_name.c_str());
  cell.selectionStyle = UITableViewCellSelectionStyleNone;
  if ([indexPath isEqual:self.selectedIndexPath])
    cell.accessoryType = UITableViewCellAccessoryCheckmark;
  else
    cell.accessoryType = UITableViewCellAccessoryNone;
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * selectedCell = [tableView cellForRowAtIndexPath:self.selectedIndexPath];
  selectedCell.accessoryType = UITableViewCellAccessoryNone;
  selectedCell = [self.tableView cellForRowAtIndexPath:indexPath];
  selectedCell.accessoryType = UITableViewCellAccessoryCheckmark;
  self.selectedIndexPath = indexPath;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return self.isSearch ? 1 : 1 /* m_categories.m_allSorted.size() > 0  by default */ + (m_categories.m_lastUsed.size() > 0 ? 1 : 0);
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self dataSourceForSection:section].size();
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (self.isSearch)
    return nil;
  if (m_categories.m_lastUsed.size() == 0)
    return L(@"all_categories_header");
  return section == 0 ? L(@"recent_categories_header") : L(@"all_categories_header");
}

- (vector<Category> const &)dataSourceForSection:(NSInteger)section
{
  if (self.isSearch)
  {
    return m_filtredCategories;
  }
  else
  {
    if (m_categories.m_lastUsed.size() > 0)
      return section == 0 ? m_categories.m_lastUsed : m_categories.m_allSorted;
    else
      return m_categories.m_allSorted;
  }
}

#pragma mark - UISearchBarDelegate

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText
{
  m_filtredCategories.clear();
  if (!searchText.length)
  {
    self.isSearch = NO;
    [self.tableView reloadData];
    return;
  }

  self.isSearch = YES;
  NSLocale * locale = [NSLocale currentLocale];
  string const query {[searchText lowercaseStringWithLocale:locale].UTF8String};

  auto const & all = m_categories.m_allSorted;
  copy_if(all.begin(), all.end(), back_inserter(m_filtredCategories), [&query](Category const & c)
  {
    string s {c.m_name};
    transform(s.begin(), s.end(), s.begin(), tolower);
    return s.find(query) != string::npos;
  });

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
  self.isSearch = isActiveState;
  if (!isActiveState)
    m_filtredCategories.clear();
}

@end
