#import "MWMObjectsCategorySelectorController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMCommon.h"
#import "MWMEditorViewController.h"
#import "MWMKeyboard.h"
#import "MWMTableViewCell.h"
#import "MWMToast.h"
#import "Statistics.h"
#import "SwiftBridge.h"
#import "UIViewController+Navigation.h"

#include "LocaleTranslator.h"

#include "Framework.h"

#include "indexer/new_feature_categories.hpp"

using namespace osm;

namespace
{
NSString * const kToEditorSegue = @"CategorySelectorToEditorSegue";

string locale()
{
  return locale_translator::bcp47ToTwineLanguage(NSLocale.currentLocale.localeIdentifier);
}

}  // namespace

@interface MWMObjectsCategorySelectorController ()<UISearchBarDelegate, UITableViewDelegate,
                                                   UITableViewDataSource, MWMKeyboardObserver>
{
  NewFeatureCategories m_categories;
  NewFeatureCategories::TNames m_filteredCategories;
}

@property(weak, nonatomic) IBOutlet UITableView * tableView;
@property(weak, nonatomic) IBOutlet UISearchBar * searchBar;
@property(nonatomic) NSIndexPath * selectedIndexPath;
@property(nonatomic) BOOL isSearch;

@end

@implementation MWMObjectsCategorySelectorController

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
  {
    m_categories = GetFramework().GetEditorCategories();
    m_categories.AddLanguage(locale());
  }
  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.isSearch = NO;
  [self configTable];
  [self configNavBar];
  [self configSearchBar];
  [MWMKeyboard addObserver:self];
}

- (void)configTable
{
  self.tableView.backgroundColor = [UIColor pressBackground];
  self.tableView.separatorColor = [UIColor blackDividers];
  [self.tableView registerClass:[MWMTableViewCell class]
         forCellReuseIdentifier:[UITableViewCell className]];
}

- (void)setSelectedCategory:(string const &)category
{
  auto const & all = m_categories.GetAllCategoryNames(locale());
  auto const it = find_if(
      all.begin(), all.end(),
      [&category](NewFeatureCategories::TName const & name) { return name.first == category; });
  NSAssert(it != all.end(), @"Incorrect category!");
  self.selectedIndexPath = [NSIndexPath indexPathForRow:(distance(all.begin(), it)) inSection:0];
}

- (void)backTap
{
  id<MWMObjectsCategorySelectorDelegate> delegate = self.delegate;
  if (delegate)
  {
    auto const object = self.createdObject;
    [delegate reloadObject:object];
  }
  [super backTap];
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
  if ([MWMToast affectsStatusBar])
    return [MWMToast preferredStatusBarStyle];
  setStatusBarBackgroundColor(UIColor.clearColor);
  return UIStatusBarStyleLightContent;
}

- (void)configNavBar { self.title = L(@"editor_add_select_category"); }
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
  MWMEditorViewController * dest =
      static_cast<MWMEditorViewController *>(segue.destinationViewController);
  dest.isCreating = YES;
  auto const object = self.createdObject;
  [dest setEditableMapObject:object];

  using namespace osm_auth_ios;
  auto const & featureID = object.GetID();
  [Statistics logEvent:kStatEditorAddStart
        withParameters:@{
          kStatIsAuthenticated: @(AuthorizationHaveCredentials()),
          kStatIsOnline: Platform::IsConnected() ? kStatYes : kStatNo,
          kStatEditorMWMName: @(featureID.GetMwmName().c_str()),
          kStatEditorMWMVersion: @(featureID.GetMwmVersion())
        }];
}

#pragma mark - MWMKeyboard

- (void)onKeyboardAnimation
{
  UIEdgeInsets const contentInsets = {.bottom = [MWMKeyboard keyboardHeight]};
  self.tableView.contentInset = contentInsets;
  self.tableView.scrollIndicatorInsets = contentInsets;
}

#pragma mark - Create object

- (EditableMapObject)createdObject
{
  auto const & ds = [self dataSourceForSection:self.selectedIndexPath.section];
  EditableMapObject emo;
  auto & f = GetFramework();
  if (!f.CreateMapObject(f.GetViewportCenter(), ds[self.selectedIndexPath.row].second, emo))
    NSAssert(false, @"This call should never fail, because IsPointCoveredByDownloadedMaps is "
                    @"always called before!");
  return emo;
}

#pragma mark - UITableView

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto cell =
      [tableView dequeueReusableCellWithCellClass:[UITableViewCell class] indexPath:indexPath];
  cell.textLabel.text =
      @([self dataSourceForSection:indexPath.section][indexPath.row].first.c_str());
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

// TODO(Vlad): Uncoment this line when we will be ready to show recent categories
//- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
//{
//  return self.isSearch ? 1 : !m_categories.m_allSorted.empty() + !m_categories.m_lastUsed.empty();
//}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self dataSourceForSection:section].size();
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  if (self.isSearch)
    return nil;
  return L(@"editor_add_select_category_all_subtitle");
  // TODO(Vlad): Uncoment this line when we will be ready to show recent categories
  //  if (m_categories.m_lastUsed.empty())
  //    return L(@"editor_add_select_category_all_subtitle");
  //  return section == 0 ? L(@"editor_add_select_category_popular_subtitle") :
  //  L(@"editor_add_select_category_all_subtitle");
}

- (NewFeatureCategories::TNames const &)dataSourceForSection:(NSInteger)section
{
  if (self.isSearch)
    return m_filteredCategories;
  return m_categories.GetAllCategoryNames(locale());
  // TODO(Vlad): Uncoment this line when we will be ready to show recent categories
  //    if (m_categories.m_lastUsed.empty())
  //      return m_categories.m_allSorted;
  //    else
  //      return section == 0 ? m_categories.m_lastUsed : m_categories.m_allSorted;
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
  string const query{[searchText lowercaseStringWithLocale:NSLocale.currentLocale].UTF8String};
  m_filteredCategories = m_categories.Search(query, locale());
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

- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar { [searchBar resignFirstResponder]; }
- (UIBarPosition)positionForBar:(id<UIBarPositioning>)bar { return UIBarPositionTopAttached; }
- (void)searchBar:(UISearchBar *)searchBar setActiveState:(BOOL)isActiveState
{
  [searchBar setShowsCancelButton:isActiveState animated:YES];
  [self.navigationController setNavigationBarHidden:isActiveState animated:YES];
  if (!isActiveState)
    m_filteredCategories.clear();
}

@end
