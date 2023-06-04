#import "MWMObjectsCategorySelectorController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMObjectsCategorySelectorDataSource.h"
#import "MWMEditorViewController.h"
#import "MWMKeyboard.h"
#import "MWMTableViewCell.h"
#import "SwiftBridge.h"

#include <CoreApi/Framework.h>

using namespace osm;

namespace
{
NSString * const kToEditorSegue = @"CategorySelectorToEditorSegue";
}  // namespace

@interface MWMObjectsCategorySelectorController ()<UISearchBarDelegate, UITableViewDelegate,
                                                   UITableViewDataSource, MWMKeyboardObserver>
{
}

@property(weak, nonatomic) IBOutlet UITableView * tableView;
@property(weak, nonatomic) IBOutlet UISearchBar * searchBar;
@property(nonatomic) NSString * selectedType;
@property(nonatomic) BOOL isSearch;
@property(nonatomic) MWMObjectsCategorySelectorDataSource * dataSource;

@end

@implementation MWMObjectsCategorySelectorController

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
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
  self.dataSource = [[MWMObjectsCategorySelectorDataSource alloc] init];
}

- (void)configTable
{
  [self.tableView registerClass:[MWMTableViewCell class]
         forCellReuseIdentifier:[UITableViewCell className]];
}

- (void)setSelectedCategory:(std::string const &)type
{
  self.selectedType = @(type.c_str());
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
  return UIStatusBarStyleLightContent;
}

- (void)configNavBar { self.title = L(@"editor_add_select_category"); }
- (void)configSearchBar
{
  self.searchBar.placeholder = L(@"search");
}

- (void)onDone
{
  if (!self.selectedType)
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
  EditableMapObject emo;
  auto & f = GetFramework();
  auto const type = classif().GetTypeByReadableObjectName(self.selectedType.UTF8String);
  if (!f.CreateMapObject(f.GetViewportCenter(), type, emo))
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
  cell.textLabel.text = [self.dataSource getTranslation:indexPath.row];
  auto const type = [self.dataSource getType:indexPath.row];
  if ([type isEqualToString:self.selectedType])
    cell.accessoryType = UITableViewCellAccessoryCheckmark;
  else
    cell.accessoryType = UITableViewCellAccessoryNone;
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.selectedType = [self.dataSource getType:indexPath.row];

  id<MWMObjectsCategorySelectorDelegate> delegate = self.delegate;
  if (delegate)
  {
    auto const object = self.createdObject;
    [delegate reloadObject:object];
    [self goBack];
  }
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
  return [self.dataSource size];
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

#pragma mark - UISearchBarDelegate

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText
{
  self.isSearch = searchText.length == 0 ? NO : YES;
  [self.dataSource search:[searchText lowercaseStringWithLocale:NSLocale.currentLocale]];
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
    [self.dataSource search:@""];
}

@end
