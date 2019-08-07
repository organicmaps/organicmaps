#import "BookmarksVC.h"
#import "BookmarksSection.h"
#import "MWMBookmarksManager.h"
#import "MWMCommon.h"
#import "MWMKeyboard.h"
#import "MWMLocationObserver.h"
#import "MWMSearchNoResults.h"
#import "MWMCategoryInfoCell.h"
#import "SwiftBridge.h"
#import "TracksSection.h"

#include "Framework.h"

#include "map/bookmarks_search_params.hpp"

#include "geometry/mercator.hpp"

#include "coding/zip_creator.hpp"
#include "coding/internal/file_data.hpp"

#include <iterator>
#include <string>
#include <vector>

using namespace std;

@interface BookmarksVC() <UITableViewDataSource,
                          UITableViewDelegate,
                          UISearchBarDelegate,
                          MWMBookmarksObserver,
                          MWMLocationObserver,
                          MWMKeyboardObserver,
                          BookmarksSectionDelegate,
                          TracksSectionDelegate,
                          MWMCategoryInfoCellDelegate,
                          BookmarksSharingViewControllerDelegate,
                          CategorySettingsViewControllerDelegate>
{
  NSMutableArray<id<TableSectionDataSource>> * m_sectionsCollection;
  BookmarkManager::SortedBlocksCollection m_sortedBlocks;
  search::BookmarksSearchParams::Results m_searchResults;
}

@property(nonatomic) NSUInteger lastSearchId;
@property(nonatomic) NSUInteger lastSortId;

@property(nonatomic) BOOL infoExpanded;

@property(weak, nonatomic) IBOutlet UIView * statusBarBackground;
@property(weak, nonatomic) IBOutlet UISearchBar * searchBar;
@property(weak, nonatomic) IBOutlet UIView * noResultsContainer;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * noResultsBottom;
@property(nonatomic) MWMSearchNoResults * noResultsView;

@property(nonatomic) UIActivityIndicatorView * spinner;
@property(nonatomic) UIImageView * searchIcon;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * hideSearchBar;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * showSearchBar;

@property(weak, nonatomic) IBOutlet UITableView * tableView;

@property(weak, nonatomic) IBOutlet UIToolbar * myCategoryToolbar;
@property(weak, nonatomic) IBOutlet UIToolbar * downloadedCategoryToolbar;
@property(weak, nonatomic) IBOutlet UIBarButtonItem * viewOnMapItem;

@property(nonatomic) UIActivityIndicatorView * sortSpinner;
@property(weak, nonatomic) IBOutlet UIBarButtonItem * sortItem;
@property(weak, nonatomic) IBOutlet UIBarButtonItem * sortSpinnerItem;

@property(weak, nonatomic) IBOutlet UIBarButtonItem * moreItem;

@end

@implementation BookmarksVC

- (instancetype)initWithCategory:(MWMMarkGroupID)index
{
  self = [super init];
  if (self)
  {
    m_categoryId = index;
    m_sectionsCollection = [NSMutableArray array];
    [self calculateSections];
  }
  return self;
}

- (BOOL)isSearchMode
{
  return !m_searchResults.empty();
}

- (BOOL)isSortMode
{
  return ![self isSearchMode] && !m_sortedBlocks.empty();
}

- (void)calculateSections
{
  [m_sectionsCollection removeAllObjects];
  
  if ([self isSearchMode])
  {
    [m_sectionsCollection addObject:[[BookmarksSection alloc] initWithDelegate:self]];
    return;
  }
  
  if ([self isSortMode])
  {
    NSInteger blockIndex = 0;
    for (auto const & block : m_sortedBlocks)
    {
      if (!block.m_markIds.empty())
        [m_sectionsCollection addObject:[[BookmarksSection alloc] initWithBlockIndex:@(blockIndex++) delegate:self]];
    }
    return;
  }
  
  auto const & bm = GetFramework().GetBookmarkManager();
  if (bm.IsCategoryFromCatalog(m_categoryId))
  {
    [m_sectionsCollection addObject:[[InfoSection alloc] initWithDelegate:self]];
  }
  
  if (bm.GetTrackIds(m_categoryId).size() > 0)
  {
    [m_sectionsCollection addObject:[[TracksSection alloc] initWithDelegate:self]];
  }
  
  if (bm.GetUserMarkIds(m_categoryId).size() > 0)
    [m_sectionsCollection addObject:[[BookmarksSection alloc] initWithDelegate:self]];
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  
  UIColor * searchBarColor = [UIColor primary];
  self.searchBar.delegate = self;
  self.statusBarBackground.backgroundColor = self.searchBar.barTintColor = searchBarColor;
  self.searchBar.backgroundImage = [UIImage imageWithColor:searchBarColor];
  self.searchBar.placeholder = L(@"search");
  
  auto const & bm = GetFramework().GetBookmarkManager();
  auto const searchAllowed = !bm.IsCategoryFromCatalog(m_categoryId);

  self.showSearchBar.priority = searchAllowed ? UILayoutPriorityRequired : UILayoutPriorityDefaultLow;
  self.hideSearchBar.priority = searchAllowed ? UILayoutPriorityDefaultLow : UILayoutPriorityRequired;

  [self.noResultsView setTranslatesAutoresizingMaskIntoConstraints:NO];
  
  self.tableView.estimatedRowHeight = 44;
  [self.tableView registerWithCellClass:MWMCategoryInfoCell.class];
  self.tableView.separatorColor = [UIColor blackDividers];

  auto regularTitleAttributes = @{ NSFontAttributeName: [UIFont regular16],
                                   NSForegroundColorAttributeName: [UIColor linkBlue] };
  auto moreTitleAttributes = @{ NSFontAttributeName: [UIFont medium16],
                                   NSForegroundColorAttributeName: [UIColor linkBlue] };

  [self.moreItem setTitleTextAttributes:moreTitleAttributes forState:UIControlStateNormal];
  [self.sortItem setTitleTextAttributes:regularTitleAttributes forState:UIControlStateNormal];
  [self.viewOnMapItem setTitleTextAttributes:regularTitleAttributes forState:UIControlStateNormal];

  self.moreItem.title = L(@"placepage_more_button");
  self.sortItem.title = L(@"sort");
  self.sortSpinnerItem.customView = self.sortSpinner;
  self.viewOnMapItem.title = L(@"search_show_on_map");

  self.myCategoryToolbar.barTintColor = [UIColor white];
  self.downloadedCategoryToolbar.barTintColor = [UIColor white];
  
  [self showSpinner:NO];
}

- (void)viewWillAppear:(BOOL)animated
{
  [MWMLocationManager addObserver:self];

  auto const & bm = GetFramework().GetBookmarkManager();
  
  // Display Edit button only if table is not empty
  if ([[MWMBookmarksManager sharedManager] isCategoryEditable:m_categoryId])
  {
    self.myCategoryToolbar.hidden = NO;
    self.downloadedCategoryToolbar.hidden = YES;
    if ([[MWMBookmarksManager sharedManager] isCategoryNotEmpty:m_categoryId])
    {
      self.navigationItem.rightBarButtonItem = self.editButtonItem;
      self.sortItem.enabled = YES;
      
      BookmarkManager::SortingType type;
      if (bm.GetLastSortingType(m_categoryId, type))
      {
        auto const availableSortingTypes = [self getAvailableSortingTypes];
        for (auto availableType : availableSortingTypes)
        {
          if (availableType == type)
          {
            [self sort:type];
            break;
          }
        }
      }
    }
    else
    {
      self.sortItem.enabled = NO;
    }
  }
  else
  {
    self.myCategoryToolbar.hidden = YES;
    self.downloadedCategoryToolbar.hidden = NO;
  }

  [super viewWillAppear:animated];

  self.title = @(bm.GetCategoryName(m_categoryId).c_str());
}

- (void)viewWillDisappear:(BOOL)animated
{
  [MWMLocationManager removeObserver:self];

  // Save possibly edited set name
  [super viewWillDisappear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
  // Disable all notifications in BM on appearance of this view.
  // It allows to significantly improve performance in case of bookmarks
  // modification. All notifications will be sent on controller's disappearance.
  [[MWMBookmarksManager sharedManager] setNotificationsEnabled: NO];
  
  [super viewDidAppear:animated];
}

- (void)viewDidDisappear:(BOOL)animated
{
  // Allow to send all notifications in BM.
  [[MWMBookmarksManager sharedManager] setNotificationsEnabled: YES];
  
  [super viewDidDisappear:animated];
}

- (IBAction)onMore:(UIBarButtonItem *)sender
{
  auto actionSheet = [UIAlertController alertControllerWithTitle:nil
                                                         message:nil
                                                  preferredStyle:UIAlertControllerStyleActionSheet];
  
  [actionSheet addAction:[UIAlertAction actionWithTitle:L(@"sharing_options")
                                                  style:UIAlertActionStyleDefault
                                                handler:^(UIAlertAction * _Nonnull action)
                          {
                            [self shareCategory];
                            [Statistics logEvent:kStatBookmarksListItemSettings withParameters:@{kStatOption : kStatSharingOptions}];
                          }]];
  
  [actionSheet addAction:[UIAlertAction actionWithTitle:L(@"search_show_on_map")
                                                  style:UIAlertActionStyleDefault
                                                handler:^(UIAlertAction * _Nonnull action)
                          {
                            [self viewOnMap];
                            [Statistics logEvent:kStatBookmarksListItemMoreClick withParameters:@{kStatOption : kStatViewOnMap}];
                          }]];

  [actionSheet addAction:[UIAlertAction actionWithTitle:L(@"list_settings")
                                                  style:UIAlertActionStyleDefault
                                                handler:^(UIAlertAction * _Nonnull action)
                          {
                            [self openCategorySettings];
                            [Statistics logEvent:kStatBookmarksListItemMoreClick withParameters:@{kStatOption : kStatSettings}];
                          }]];

  [actionSheet addAction:[UIAlertAction actionWithTitle:L(@"export_file")
                                                  style:UIAlertActionStyleDefault
                                                handler:^(UIAlertAction * _Nonnull action)
                          {
                            [self exportFile];
                            [Statistics logEvent:kStatBookmarksListItemMoreClick withParameters:@{kStatOption : kStatSendAsFile}];
                          }]];

  auto deleteAction = [UIAlertAction actionWithTitle:L(@"delete_list")
                                               style:UIAlertActionStyleDestructive
                                             handler:^(UIAlertAction * _Nonnull action)
                       {
                         [[MWMBookmarksManager sharedManager] deleteCategory:self->m_categoryId];
                         [self.delegate bookmarksVCdidDeleteCategory:self];
                         [Statistics logEvent:kStatBookmarksListItemMoreClick withParameters:@{kStatOption : kStatDelete}];
                       }];
  deleteAction.enabled = [[MWMBookmarksManager sharedManager] userCategories].count > 1;
  [actionSheet addAction:deleteAction];

  [actionSheet addAction:[UIAlertAction actionWithTitle:L(@"cancel")
                                                  style:UIAlertActionStyleCancel
                                                handler:nil]];

  actionSheet.popoverPresentationController.barButtonItem = self.moreItem;
  [self presentViewController:actionSheet animated:YES completion:^{
    actionSheet.popoverPresentationController.passthroughViews = nil;
  }];
  [Statistics logEvent:kStatBookmarksListItemSettings withParameters:@{kStatOption : kStatMore}];
}

- (IBAction)onViewOnMap:(UIBarButtonItem *)sender
{
  [self viewOnMap];
}

- (void)openCategorySettings
{
  auto storyboard = [UIStoryboard instance:MWMStoryboardCategorySettings];
  auto settingsController = (CategorySettingsViewController *)[storyboard instantiateInitialViewController];
  settingsController.delegate = self;
  settingsController.category = [[MWMBookmarksManager sharedManager] categoryWithId:m_categoryId];
  [self.navigationController pushViewController:settingsController animated:YES];
}

- (void)exportFile
{
  [[MWMBookmarksManager sharedManager] addObserver:self];
  [[MWMBookmarksManager sharedManager] shareCategory:m_categoryId];
}

- (void)shareCategory
{
  auto storyboard = [UIStoryboard instance:MWMStoryboardSharing];
  auto shareController = (BookmarksSharingViewController *)[storyboard instantiateInitialViewController];
  shareController.delegate = self;
  shareController.category = [[MWMBookmarksManager sharedManager] categoryWithId:m_categoryId];
  [self.navigationController pushViewController:shareController animated:YES];
}

- (void)viewOnMap
{
  [self.navigationController popToRootViewControllerAnimated:YES];
  GetFramework().ShowBookmarkCategory(m_categoryId);
}

- (IBAction)onSort:(UIBarButtonItem *)sender
{
  auto actionSheet = [UIAlertController alertControllerWithTitle:nil
                                                         message:nil
                                                  preferredStyle:UIAlertControllerStyleActionSheet];
  
  auto const sortingTypes = [self getAvailableSortingTypes];
  
  for (auto type : sortingTypes)
  {
    [actionSheet addAction:[UIAlertAction actionWithTitle:[BookmarksVC getLocalizedSortingType:type]
                                                    style:UIAlertActionStyleDefault
                                                  handler:^(UIAlertAction * _Nonnull action)
                            {
                              auto & bm = GetFramework().GetBookmarkManager();
                              bm.SetLastSortingType(self->m_categoryId, type);
                              [self sort:type];
                            }]];
  }
  
  [actionSheet addAction:[UIAlertAction actionWithTitle:L(@"sort_default")
                                                  style:UIAlertActionStyleDefault
                                                handler:^(UIAlertAction * _Nonnull action)
                          {
                            [self sortDefault];
                          }]];
  
  [actionSheet addAction:[UIAlertAction actionWithTitle:L(@"cancel")
                                                  style:UIAlertActionStyleCancel
                                                handler:nil]];
  
  actionSheet.popoverPresentationController.barButtonItem = self.sortItem;
  [self presentViewController:actionSheet animated:YES completion:^{
    actionSheet.popoverPresentationController.passthroughViews = nil;
  }];
}

+ (NSString *)getLocalizedSortingType:(BookmarkManager::SortingType)type
{
  switch (type)
  {
    case BookmarkManager::SortingType::ByTime: return L(@"sort_date");
    case BookmarkManager::SortingType::ByDistance: return L(@"sort_distance");
    case BookmarkManager::SortingType::ByType: return L(@"sort_type");
  }
  UNREACHABLE();
}

- (std::vector<BookmarkManager::SortingType>)getAvailableSortingTypes
{
  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  bool const hasMyPosition = lastLocation != nil;
  auto const & bm = GetFramework().GetBookmarkManager();
  auto const sortingTypes = bm.GetAvailableSortingTypes(m_categoryId, hasMyPosition);
  return sortingTypes;
}

- (void)sortDefault
{
  auto & bm = GetFramework().GetBookmarkManager();
  bm.ResetLastSortingType(self->m_categoryId);
  self->m_sortedBlocks.clear();
  [self calculateSections];
  [self.tableView reloadData];
}

- (void)sort:(BookmarkManager::SortingType)type
{
  bool hasMyPosition = false;
  m2::PointD myPosition = m2::PointD::Zero();
  
  if (type == BookmarkManager::SortingType::ByDistance)
  {
    CLLocation * lastLocation = [MWMLocationManager lastLocation];
    if (!lastLocation)
      return;
    hasMyPosition = true;
    myPosition = MercatorBounds::FromLatLon(lastLocation.coordinate.latitude, lastLocation.coordinate.longitude);
  }
  
  auto const sortId = ++self.lastSortId;
  __weak auto weakSelf = self;
  
  auto & bm = GetFramework().GetBookmarkManager();
  BookmarkManager::SortParams sortParams;
  sortParams.m_groupId = m_categoryId;
  sortParams.m_sortingType = type;
  sortParams.m_hasMyPosition = hasMyPosition;
  sortParams.m_myPosition = myPosition;
  sortParams.m_onResults = [weakSelf, sortId](BookmarkManager::SortedBlocksCollection && sortedBlocks,
                                              BookmarkManager::SortParams::Status status) {
    __strong auto self = weakSelf;
    if (!self || sortId != self.lastSortId)
      return;
    
    [self showSortSpinner:NO];
    self.sortItem.enabled = YES;
    
    if (status == BookmarkManager::SortParams::Status::Completed)
    {
      m_sortedBlocks = std::move(sortedBlocks);
      [self calculateSections];
      [self.tableView reloadData];
    }
  };
  
  [self showSortSpinner:YES];
  self.sortItem.enabled = NO;
  
  bm.GetSortedBookmarks(sortParams);
}

- (BookmarkManager::SortedBlock &)sortedBlockForIndex:(NSNumber *)blockIndex
{
  CHECK(blockIndex != nil, ());
  NSInteger index = blockIndex.integerValue;
  CHECK_LESS(index, m_sortedBlocks.size(), ());
  return m_sortedBlocks[index];
}

- (void)deleteSortedBlockForIndex:(NSNumber *)blockIndex
{
  CHECK(blockIndex != nil, ());
  NSInteger index = blockIndex.integerValue;
  CHECK_LESS(index, m_sortedBlocks.size(), ());
  m_sortedBlocks.erase(m_sortedBlocks.begin() + index);
}

- (UIActivityIndicatorView *)sortSpinner
{
  if (!_sortSpinner)
  {
    _sortSpinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    _sortSpinner.hidesWhenStopped = YES;
  }
  return _sortSpinner;
}

- (void)showSortSpinner:(BOOL)show
{
  if (show)
    [self.sortSpinner startAnimating];
  else
    [self.sortSpinner stopAnimating];
}

- (void)cancelSearch
{
  GetFramework().CancelSearch(search::Mode::Bookmarks);
  m_searchResults.clear();
  
  [self showNoResultsView:NO];
  [self showSpinner:NO];
  
  [self calculateSections];
  [self.tableView reloadData];
}

- (MWMSearchNoResults *)noResultsView
{
  if (!_noResultsView)
  {
    _noResultsView = [MWMSearchNoResults viewWithImage:[UIImage imageNamed:@"img_search_not_found"]
                                                 title:L(@"search_not_found")
                                                  text:L(@"search_not_found_query")];
  }
  return _noResultsView;
}

- (void)showNoResultsView:(BOOL)show
{
  if (!show)
  {
    self.tableView.hidden = NO;
    self.noResultsContainer.hidden = YES;
    [self.noResultsView removeFromSuperview];
  }
  else
  {
    self.tableView.hidden = YES;
    self.noResultsContainer.hidden = NO;
    [self.noResultsContainer addSubview:self.noResultsView];
    self.noResultsView.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
                                              [self.noResultsView.topAnchor constraintEqualToAnchor:self.noResultsContainer.topAnchor],
                                              [self.noResultsView.leftAnchor constraintEqualToAnchor:self.noResultsContainer.leftAnchor],
                                              [self.noResultsView.bottomAnchor constraintEqualToAnchor:self.noResultsContainer.bottomAnchor],
                                              [self.noResultsView.rightAnchor constraintEqualToAnchor:self.noResultsContainer.rightAnchor],
                                              ]];
    [self onKeyboardAnimation];
  }
}

- (UIActivityIndicatorView *)spinner
{
  if (!_spinner)
  {
    _spinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    _spinner.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
    _spinner.hidesWhenStopped = YES;
  }
  return _spinner;
}

- (UIImageView *)searchIcon
{
  if (!_searchIcon)
  {
    _searchIcon = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"ic_search"]];
    _searchIcon.mwm_coloring = MWMImageColoringBlack;
  }
  return _searchIcon;
}

- (void)showSpinner:(BOOL)show
{
  UITextField * textField = [self.searchBar valueForKey:@"searchField"];
  if (!show)
  {
    textField.leftView = self.searchIcon;
    [self.spinner stopAnimating];
  }
  else
  {
    self.spinner.bounds = textField.leftView.bounds;
    textField.leftView = self.spinner;
    [self.spinner startAnimating];
  }
}

- (NSString *)categoryFileName
{
  return @(GetFramework().GetBookmarkManager().GetCategoryFileName(m_categoryId).c_str());
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
  setStatusBarBackgroundColor(UIColor.clearColor);
  return UIStatusBarStyleLightContent;
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated
{
  [super setEditing:editing animated:animated];
  [self.tableView setEditing:editing animated:animated];
}

#pragma mark - UISearchBarDelegate

- (BOOL)searchBarShouldBeginEditing:(UISearchBar *)searchBar
{
  [self.searchBar setShowsCancelButton:YES animated:YES];
  [self.navigationController setNavigationBarHidden:YES animated:YES];
  self.tableView.contentInset = self.tableView.scrollIndicatorInsets = {};
  
  // Allow to send all notifications in BM.
  [[MWMBookmarksManager sharedManager] setNotificationsEnabled: YES];
  
  return YES;
}

- (BOOL)searchBarShouldEndEditing:(UISearchBar *)searchBar
{
  [self.searchBar setShowsCancelButton:NO animated:YES];
  [self.navigationController setNavigationBarHidden:NO animated:YES];
  self.tableView.contentInset = self.tableView.scrollIndicatorInsets = {};
  
  // Disable all notifications in BM.
  [[MWMBookmarksManager sharedManager] setNotificationsEnabled: NO];
  
  return YES;
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar
{
  self.searchBar.text = @"";
  [self.searchBar resignFirstResponder];
  [self cancelSearch];
}

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText
{
  if (!searchText || searchText.length == 0)
  {
    [self cancelSearch];
    return;
  }
  
  search::BookmarksSearchParams searchParams;
  searchParams.m_query = searchText.UTF8String;
  searchParams.m_groupId = m_categoryId;
  
  auto const searchId = ++self.lastSearchId;
  __weak auto weakSelf = self;
  searchParams.m_onStarted = [] {};
  searchParams.m_onResults = [weakSelf, searchId](search::BookmarksSearchParams::Results const & results,
                                                  search::BookmarksSearchParams::Status status) {
    __strong auto self = weakSelf;
    if (!self || searchId != self.lastSearchId)
      return;
    
    auto const & bm = GetFramework().GetBookmarkManager();
    auto filteredResults = results;
    bm.FilterInvalidBookmarks(filteredResults);
    self->m_searchResults = filteredResults;
    
    if (status == search::BookmarksSearchParams::Status::Cancelled)
    {
      [self showSpinner:NO];
    }
    else if (status == search::BookmarksSearchParams::Status::Completed)
    {
      [self showNoResultsView:results.empty()];
      [self showSpinner:NO];
    }
    
    [self calculateSections];
    [self.tableView reloadData];
  };
  
  [self showSpinner:YES];
  
  GetFramework().SearchInBookmarks(searchParams);
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return [m_sectionsCollection count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [m_sectionsCollection[section] numberOfRows];
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  return [m_sectionsCollection[section] title];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = [m_sectionsCollection[indexPath.section] tableView:tableView cellForRow:indexPath.row];
  
  cell.backgroundColor = [UIColor white];
  cell.textLabel.textColor = [UIColor blackPrimaryText];
  cell.detailTextLabel.textColor = [UIColor blackSecondaryText];
  return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  // Remove cell selection
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];
  
  auto const close = [m_sectionsCollection[indexPath.section] didSelectRow:indexPath.row];
  
  [self.searchBar resignFirstResponder];
  
  if (close)
    [self.navigationController popToRootViewControllerAnimated:NO];
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
  return [m_sectionsCollection[indexPath.section] canEdit];
}

- (void)tableView:(UITableView *)tableView willBeginEditingRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.editing = YES;
}

- (void)tableView:(UITableView *)tableView didEndEditingRowAtIndexPath:(NSIndexPath *)indexPath
{
  self.editing = NO;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (![m_sectionsCollection[indexPath.section] canEdit])
    return;
  
  BOOL emptySection = NO;
  if (editingStyle == UITableViewCellEditingStyleDelete)
    emptySection = [m_sectionsCollection[indexPath.section] deleteRow:indexPath.row];
  
  [self calculateSections];
  
  // We can delete the row with animation, if the sections stay the same.
  if (!emptySection)
    [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
  else
    [self.tableView reloadData];
  
  auto const & bm = GetFramework().GetBookmarkManager();
  if (bm.GetUserMarkIds(m_categoryId).size() + bm.GetTrackIds(m_categoryId).size() == 0)
  {
    self.navigationItem.rightBarButtonItem = nil;
    [self setEditing:NO animated:YES];
  }
}

- (void)tableView:(UITableView *)tableView willDisplayHeaderView:(UIView *)view forSection:(NSInteger)section
{
  auto header = (UITableViewHeaderFooterView *)view;
  header.textLabel.textColor = [UIColor blackSecondaryText];
  header.textLabel.font = [UIFont medium14];
}

#pragma mark - BookmarksSectionDelegate

- (NSInteger)numberOfBookmarksInSection:(BookmarksSection *)bookmarkSection
{
  if ([self isSearchMode])
  {
    return m_searchResults.size();
  }
  
  if ([self isSortMode])
  {
    auto const & sortedBlock = [self sortedBlockForIndex:bookmarkSection.blockIndex];
    return sortedBlock.m_markIds.size();
  }
  auto const & bm = GetFramework().GetBookmarkManager();
  return bm.GetUserMarkIds(m_categoryId).size();
}

- (NSString *)titleOfBookmarksSection:(BookmarksSection *)bookmarkSection
{
  if ([self isSearchMode])
    return nil;
  
  if ([self isSortMode])
  {
    auto const & sortedBlock = [self sortedBlockForIndex:bookmarkSection.blockIndex];
    return @(sortedBlock.m_blockName.c_str());
  }
  
  return L(@"bookmarks");
}

- (BOOL)canEditBookmarksSection:(BookmarksSection *)bookmarkSection
{
  return [[MWMBookmarksManager sharedManager] isCategoryEditable:m_categoryId];
}

- (kml::MarkId)bookmarkSection:(BookmarksSection *)bookmarkSection getBookmarkIdByRow:(NSInteger)row
{
  if ([self isSearchMode])
  {
    CHECK_LESS(row, m_searchResults.size(), ());
    return m_searchResults[row];
  }
  
  if ([self isSortMode])
  {
    auto const & sortedBlock = [self sortedBlockForIndex:bookmarkSection.blockIndex];
    CHECK_LESS(row, sortedBlock.m_markIds.size(), ());
    return sortedBlock.m_markIds[row];
  }
  
  auto const & bm = GetFramework().GetBookmarkManager();
  auto const & bookmarkIds = bm.GetUserMarkIds(m_categoryId);
  CHECK_LESS(row, bookmarkIds.size(), ());
  auto it = bookmarkIds.begin();
  advance(it, row);
  return *it;
}

- (BOOL)bookmarkSection:(BookmarksSection *)bookmarkSection onDeleteBookmarkInRow:(NSInteger)row
{
  if ([self isSearchMode])
  {
    CHECK_LESS(row, m_searchResults.size(), ());
    m_searchResults.erase(m_searchResults.begin() + row);
    m_sortedBlocks.clear();
    return m_searchResults.empty();
  }
  
  if ([self isSortMode])
  {
    auto & sortedBlock = [self sortedBlockForIndex:bookmarkSection.blockIndex];
    auto & marks = sortedBlock.m_markIds;
    CHECK_LESS(row, marks.size(), ());
    
    marks.erase(marks.begin() + row);
    if (marks.empty())
    {
      [self deleteSortedBlockForIndex:bookmarkSection.blockIndex];
      return YES;
    }
    return NO;
  }
  
  auto const & bm = GetFramework().GetBookmarkManager();
  auto const & bookmarkIds = bm.GetUserMarkIds(m_categoryId);
  return bookmarkIds.empty();
}

#pragma mark - TracksSectionDelegate

- (NSInteger)numberOfTracksInSection:(TracksSection *)tracksSection
{
  CHECK(![self isSearchMode], ());
  
  if ([self isSortMode])
  {
    auto const & sortedBlock = [self sortedBlockForIndex:tracksSection.blockIndex];
    return sortedBlock.m_trackIds.size();
  }
  
  auto const & bm = GetFramework().GetBookmarkManager();
  return bm.GetTrackIds(m_categoryId).size();
}

- (NSString *)titleOfTracksSection:(TracksSection *)tracksSection
{
  CHECK(![self isSearchMode], ());
  
  if ([self isSortMode])
  {
    auto const & sortedBlock = [self sortedBlockForIndex:tracksSection.blockIndex];
    return @(sortedBlock.m_blockName.c_str());
  }
  
  return L(@"tracks_title");
}

- (BOOL)canEditTracksSection:(TracksSection *)tracksSection
{
  CHECK(![self isSearchMode], ());
  
  if ([self isSortMode])
    return false;
  
  return [[MWMBookmarksManager sharedManager] isCategoryEditable:m_categoryId];
}

- (kml::TrackId)tracksSection:(TracksSection *)tracksSection getTrackIdByRow:(NSInteger)row
{
  CHECK(![self isSearchMode], ());
  
  if ([self isSortMode])
  {
    auto const & sortedBlock = [self sortedBlockForIndex:tracksSection.blockIndex];
    CHECK_LESS(row, sortedBlock.m_trackIds.size(), ());
    return sortedBlock.m_trackIds[row];
  }
  
  auto const & bm = GetFramework().GetBookmarkManager();
  auto const & trackIds = bm.GetTrackIds(m_categoryId);
  CHECK_LESS(row, trackIds.size(), ());
  auto it = trackIds.begin();
  advance(it, row);
  return *it;
}

- (BOOL)tracksSection:(TracksSection *)tracksSection onDeleteTrackInRow:(NSInteger)row
{
  CHECK(![self isSearchMode], ());
  
  if ([self isSortMode])
  {
    auto & sortedBlock = [self sortedBlockForIndex:tracksSection.blockIndex];
    CHECK_LESS(row, sortedBlock.m_trackIds.size(), ());
    
    auto & tracks = sortedBlock.m_trackIds;
    tracks.erase(tracks.begin() + row);
    if (tracks.empty())
    {
      [self deleteSortedBlockForIndex:tracksSection.blockIndex];
      return YES;
    }
    return NO;
  }
  
  auto const & bm = GetFramework().GetBookmarkManager();
  auto const & trackIds = bm.GetTrackIds(m_categoryId);
  return trackIds.empty();
}

#pragma mark - InfoSectionDelegate

- (UITableViewCell *)infoCellForTableView: (UITableView *)tableView
{
  UITableViewCell * cell = [tableView dequeueReusableCellWithCellClass:MWMCategoryInfoCell.class];
  CHECK(cell, ("Invalid category info cell."));
  
  auto & f = GetFramework();
  auto & bm = f.GetBookmarkManager();
  bool const categoryExists = bm.HasBmCategory(m_categoryId);
  CHECK(categoryExists, ("Nonexistent category"));
  
  auto infoCell = (MWMCategoryInfoCell *)cell;
  auto const & categoryData = bm.GetCategoryData(m_categoryId);
  [infoCell updateWithCategoryData:categoryData delegate:self];
  infoCell.expanded = self.infoExpanded;
  
  return cell;
}

#pragma mark - MWMCategoryInfoCellDelegate

- (void)categoryInfoCellDidPressMore:(MWMCategoryInfoCell *)cell
{
  [self.tableView beginUpdates];
  cell.expanded = YES;
  [self.tableView endUpdates];
  self.infoExpanded = YES;
}

#pragma mark - MWMLocationObserver

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  [self.tableView.visibleCells enumerateObjectsUsingBlock:^(UITableViewCell * cell, NSUInteger idx, BOOL * stop)
   {
     auto const indexPath = [self.tableView indexPathForCell:cell];
     auto const & section = self->m_sectionsCollection[indexPath.section];
     if ([section respondsToSelector:@selector(updateCell:forRow:withNewLocation:)])
       [section updateCell:cell forRow:indexPath.row withNewLocation:info];
   }];
}

#pragma mark - MWMBookmarksObserver

- (void)onBookmarksCategoryFilePrepared:(MWMBookmarksShareStatus)status
{
  switch (status)
  {
    case MWMBookmarksShareStatusSuccess:
    {
      auto shareController =
      [MWMActivityViewController shareControllerForURL:[MWMBookmarksManager sharedManager].shareCategoryURL
                                               message:L(@"share_bookmarks_email_body")
                                     completionHandler:^(UIActivityType  _Nullable activityType,
                                                         BOOL completed,
                                                         NSArray * _Nullable returnedItems,
                                                         NSError * _Nullable activityError) {
                                       [[MWMBookmarksManager sharedManager] finishShareCategory];
                                     }];
      [shareController presentInParentViewController:self anchorView:self.view];
      break;
    }
    case MWMBookmarksShareStatusEmptyCategory:
      [[MWMAlertViewController activeAlertController] presentInfoAlert:L(@"bookmarks_error_title_share_empty")
                                                                  text:L(@"bookmarks_error_message_share_empty")];
      break;
    case MWMBookmarksShareStatusArchiveError:
    case MWMBookmarksShareStatusFileError:
      [[MWMAlertViewController activeAlertController] presentInfoAlert:L(@"dialog_routing_system_error")
                                                                  text:L(@"bookmarks_error_message_share_general")];
      break;
  }
  [[MWMBookmarksManager sharedManager] removeObserver:self];
}

#pragma mark - BookmarksSharingViewControllerDelegate

- (void)didShareCategory
{
  [self.tableView reloadData];
}

#pragma mark - CategorySettingsViewControllerDelegate

- (void)categorySettingsController:(CategorySettingsViewController *)viewController didDelete:(MWMMarkGroupID)categoryId
{
  [self.delegate bookmarksVCdidDeleteCategory:self];
}

- (void)categorySettingsController:(CategorySettingsViewController *)viewController didEndEditing:(MWMMarkGroupID)categoryId
{
  [self.navigationController popViewControllerAnimated:YES];
  [self.delegate bookmarksVCdidUpdateCategory:self];
  [self.tableView reloadData];
}

#pragma mark - MWMKeyboard

- (void)onKeyboardAnimation
{
  self.noResultsBottom.constant = 0;
  CGFloat const keyboardHeight = [MWMKeyboard keyboardHeight];
  if (keyboardHeight >= self.noResultsContainer.height)
    return;
  
  self.noResultsBottom.constant = -keyboardHeight;
}
@end
