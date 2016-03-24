#import "MWMAuthorizationCommon.h"
#import "MWMEditorViewController.h"
#import "MWMObjectsCategorySelectorController.h"
#import "MWMTableViewCell.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"
#import "UIViewController+Navigation.h"

#include "Framework.h"

#include "indexer/search_string_utils.hpp"

#include "platform/platform.hpp"

using namespace osm;

namespace
{
  NSString * const kToEditorSegue = @"CategorySelectorToEditorSegue";
} // namespace

@interface MWMObjectsCategorySelectorController () <UISearchBarDelegate, UITableViewDelegate, UITableViewDataSource>
{
  NewFeatureCategories m_categories;
  vector<Category> m_filteredCategories;
}

@property (weak, nonatomic) IBOutlet UITableView * tableView;
@property (weak, nonatomic) IBOutlet UISearchBar * searchBar;
@property (nonatomic) NSIndexPath * selectedIndexPath;
@property (nonatomic) BOOL isSearch;

@end

@implementation MWMObjectsCategorySelectorController

- (void)viewDidLoad
{
  [super viewDidLoad];
  if (m_categories.m_allSorted.empty())
    m_categories = GetFramework().GetEditorCategories();

  NSAssert(!m_categories.m_allSorted.empty(), @"Categories list can't be empty!");

  self.isSearch = NO;
  [self configTable];
  [self configNavBar];
  [self configSearchBar];

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWillShow:)
                                               name:UIKeyboardWillShowNotification
                                             object:nil];

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWillHide:)
                                               name:UIKeyboardWillHideNotification
                                             object:nil];
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)keyboardWillShow:(NSNotification *)notification
{
  CGSize const keyboardSize = [[[notification userInfo] objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
  CGFloat const bottomInset = UIInterfaceOrientationIsPortrait([[UIApplication sharedApplication] statusBarOrientation]) ?
                                                               keyboardSize.height : keyboardSize.width;

  UIEdgeInsets const contentInsets = {.bottom = bottomInset};

  NSNumber * rate = notification.userInfo[UIKeyboardAnimationDurationUserInfoKey];
  [UIView animateWithDuration:rate.floatValue animations:^
  {
    self.tableView.contentInset = contentInsets;
    self.tableView.scrollIndicatorInsets = contentInsets;
  }];
}

- (void)keyboardWillHide:(NSNotification *)notification
{
  NSNumber * rate = notification.userInfo[UIKeyboardAnimationDurationUserInfoKey];
  [UIView animateWithDuration:rate.floatValue animations:^
  {
    self.tableView.contentInset = {};
    self.tableView.scrollIndicatorInsets = {};
  }];
}

- (void)configTable
{
  self.tableView.backgroundColor = [UIColor pressBackground];
  self.tableView.separatorColor = [UIColor blackDividers];
  [self.tableView registerClass:[MWMTableViewCell class] forCellReuseIdentifier:[UITableViewCell className]];
}

- (void)setSelectedCategory:(string const &)category
{
  m_categories = GetFramework().GetEditorCategories();
  auto const & all = m_categories.m_allSorted;
  auto const it = find_if(all.begin(), all.end(), [&category](Category const & c)
  {
    return c.m_name == category;
  });
  NSAssert(it != all.end(), @"Incorrect category!");
  self.selectedIndexPath = [NSIndexPath indexPathForRow:(it - all.begin())
                                              inSection:m_categories.m_lastUsed.empty() ? 0 : 1];
}

- (void)backTap
{
  if (self.delegate)
  {
    auto const object = self.createdObject;
    [self.delegate reloadObject:object];
  }
  [super backTap];
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
  return UIStatusBarStyleLightContent;
}

- (void)configNavBar
{
  self.title = L(@"editor_add_select_category");
}

- (void)configSearchBar
{
  self.searchBar.backgroundImage = [UIImage imageWithColor:[UIColor primary]];
  self.searchBar.placeholder = L(@"search");
  UITextField * textFiled = [self.searchBar valueForKey:@"searchField"];
  UILabel * placeholder = [textFiled valueForKey:@"_placeholderLabel"];
  placeholder.textColor = [UIColor blackHintText];
}

- (void)onDone
{
  if (!self.selectedIndexPath)
    return;
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
  [dest setEditableMapObject:object];

  using namespace osm_auth_ios;
  auto const & featureID = object.GetID();
  [Statistics logEvent:kStatEditorAddStart withParameters:@{kStatEditorIsAuthenticated : @(AuthorizationHaveCredentials()),
                                                            kStatIsOnline : Platform::IsConnected() ? kStatYes : kStatNo,
                                                            kStatEditorMWMName : @(featureID.GetMwmName().c_str()),
                                                            kStatEditorMWMVersion : @(featureID.GetMwmVersion())}];
}

#pragma mark - Create object

- (EditableMapObject)createdObject
{
  auto const & ds = [self dataSourceForSection:self.selectedIndexPath.section];
  EditableMapObject emo;
  auto & f = GetFramework();
  if (!f.CreateMapObject(f.GetViewportCenter() ,ds[self.selectedIndexPath.row].m_type, emo))
    NSAssert(false, @"This call should never fail, because IsPointCoveredByDownloadedMaps is always called before!");
  return emo;
}

#pragma mark - UITableView

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:[UITableViewCell className]];
  cell.textLabel.text = @([self dataSourceForSection:indexPath.section][indexPath.row].m_name.c_str());
  if ([indexPath isEqual:self.selectedIndexPath])
    cell.accessoryType = UITableViewCellAccessoryCheckmark;
  else
    cell.accessoryType = UITableViewCellAccessoryNone;
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.selectedIndexPath = indexPath;

  if (self.delegate)
    [self backTap];
  else
    [self performSegueWithIdentifier:kToEditorSegue sender:nil];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return self.isSearch ? 1 : !m_categories.m_allSorted.empty() + !m_categories.m_lastUsed.empty();
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self dataSourceForSection:section].size();
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (self.isSearch)
    return nil;
  if (m_categories.m_lastUsed.empty())
    return L(@"editor_add_select_category_all_subtitle");
  return section == 0 ? L(@"editor_add_select_category_popular_subtitle") : L(@"editor_add_select_category_all_subtitle");
}

- (vector<Category> const &)dataSourceForSection:(NSInteger)section
{
  if (self.isSearch)
  {
    return m_filteredCategories;
  }
  else
  {
    if (m_categories.m_lastUsed.empty())
      return m_categories.m_allSorted;
    else
      return section == 0 ? m_categories.m_lastUsed : m_categories.m_allSorted;
  }
}

#pragma mark - UISearchBarDelegate

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText
{
  m_filteredCategories.clear();
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

  for (auto const & c : all)
  {
    if (search::ContainsNormalized(c.m_name, query))
      m_filteredCategories.push_back(c);
  }

  [self.tableView reloadData];
}

- (BOOL)searchBarShouldBeginEditing:(UISearchBar *)searchBar
{
  [self searchBar:searchBar setActiveState:YES];
  self.isSearch = NO;
  return YES;
}

- (BOOL)searchBarShouldEndEditing:(UISearchBar *)searchBar
{
  if (!searchBar.text.length)
  {
    [self searchBar:searchBar setActiveState:NO];
    self.isSearch = NO;
  }
  return YES;
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar
{
  [searchBar resignFirstResponder];
  searchBar.text = @"";
  [self searchBar:searchBar setActiveState:NO];
  self.isSearch = NO;
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
  if (!isActiveState)
    m_filteredCategories.clear();
}

@end
