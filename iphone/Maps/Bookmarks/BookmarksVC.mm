#import "BookmarksVC.h"
#import "BookmarksSection.h"
#import "InfoSection.h"
#import "MWMCategoryInfoCell.h"
#import "SwiftBridge.h"

#import "MWMKeyboard.h"
#import "MWMLocationObserver.h"
#import "MWMSearchNoResults.h"
#import "TracksSection.h"

#import <CoreApi/CoreApi.h>

#include "map/bookmarks_search_params.hpp"

#include "geometry/mercator.hpp"

#include "coding/zip_creator.hpp"
#include "coding/internal/file_data.hpp"

#include <iterator>
#include <string>
#include <vector>

using namespace std;

@interface BookmarksVC () <UITableViewDataSource,
                           UITableViewDelegate,
                           UISearchBarDelegate,
                           UIScrollViewDelegate,
                           MWMBookmarksObserver,
                           MWMLocationObserver,
                           MWMKeyboardObserver,
                           InfoSectionDelegate,
                           BookmarksSharingViewControllerDelegate,
                           CategorySettingsViewControllerDelegate>

@property(strong, nonatomic) NSMutableArray<id<TableSectionDataSource>> *defaultSections;
@property(strong, nonatomic) NSMutableArray<id<TableSectionDataSource>> *searchSections;
@property(strong, nonatomic) InfoSection *infoSection;

@property(nonatomic) NSUInteger lastSearchId;
@property(nonatomic) NSUInteger lastSortId;

@property(weak, nonatomic) IBOutlet UIView *statusBarBackground;
@property(weak, nonatomic) IBOutlet UISearchBar *searchBar;
@property(weak, nonatomic) IBOutlet UIView *noResultsContainer;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint *noResultsBottom;
@property(nonatomic) MWMSearchNoResults *noResultsView;

@property(nonatomic) UIActivityIndicatorView *spinner;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint *hideSearchBar;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint *showSearchBar;

@property(weak, nonatomic) IBOutlet UITableView * tableView;

@property(weak, nonatomic) IBOutlet UIToolbar * myCategoryToolbar;
@property(weak, nonatomic) IBOutlet UIToolbar * downloadedCategoryToolbar;
@property(weak, nonatomic) IBOutlet UIBarButtonItem * viewOnMapItem;

@property(nonatomic) UIActivityIndicatorView *sortSpinner;

@property(weak, nonatomic) IBOutlet UIBarButtonItem *sortItem;
@property(weak, nonatomic) IBOutlet UIBarButtonItem *sortSpinnerItem;

@property(weak, nonatomic) IBOutlet UIBarButtonItem *sortDownloadedItem;
@property(weak, nonatomic) IBOutlet UIBarButtonItem *sortDownloadedSpinnerItem;

@property(weak, nonatomic) IBOutlet UIBarButtonItem * moreItem;

@end

@implementation BookmarksVC

- (instancetype)initWithCategory:(MWMMarkGroupID)categoryId {
  self = [super init];
  if (self)
  {
    _categoryId = categoryId;
  }
  return self;
}

- (BOOL)isEditable {
  return [[MWMBookmarksManager sharedManager] isCategoryEditable:self.categoryId];
}

- (BOOL)hasInfo {
  auto const &categoryData = GetFramework().GetBookmarkManager().GetCategoryData(_categoryId);
  return !GetPreferredBookmarkStr(categoryData.m_description).empty() ||
    !GetPreferredBookmarkStr(categoryData.m_annotation).empty();
}

- (InfoSection *)cachedInfoSection
{
  if (self.infoSection == nil)
  {
    self.infoSection = [[InfoSection alloc] initWithCategoryId:self.categoryId
                                                      expanded:NO
                                                      observer:self];
  }
  return self.infoSection;
}

- (NSMutableArray<id<TableSectionDataSource>> *)currentSections {
  if (self.searchSections != nil)
    return self.searchSections;
  return self.defaultSections;
}

- (void)setCategorySections {
  if (self.defaultSections == nil)
    self.defaultSections = [[NSMutableArray alloc] init];
  else
    [self.defaultSections removeAllObjects];

  if ([self hasInfo])
    [self.defaultSections addObject:[self cachedInfoSection]];

  MWMTrackIDCollection trackIds = [[MWMBookmarksManager sharedManager] trackIdsForCategory:self.categoryId];
  if (trackIds.count > 0) {
    [self.defaultSections addObject:[[TracksSection alloc]
                                      initWithTitle:L(@"tracks_title")
                                           trackIds:trackIds
                                         isEditable:[self isEditable]]];
  }

  MWMMarkIDCollection markIds = [[MWMBookmarksManager sharedManager] bookmarkIdsForCategory:self.categoryId];
  if (markIds.count > 0) {
    [self.defaultSections addObject:[[BookmarksSection alloc] initWithTitle:L(@"bookmarks")
                                                                    markIds:markIds
                                                                 isEditable:[self isEditable]]];
  }
}

- (void)setSortedSections:(BookmarkManager::SortedBlocksCollection const &)sortResults {
  if (self.defaultSections == nil)
    self.defaultSections = [[NSMutableArray alloc] init];
  else
    [self.defaultSections removeAllObjects];

  if ([self hasInfo])
    [self.defaultSections addObject:[self cachedInfoSection]];
  
  for (auto const &block : sortResults) {
    if (!block.m_markIds.empty()) {
      [self.defaultSections addObject:[[BookmarksSection alloc] initWithTitle:@(block.m_blockName.c_str())
                                                                      markIds:[BookmarksVC bookmarkIds:block.m_markIds]
                                                                   isEditable:[self isEditable]]];
    } else if (!block.m_trackIds.empty()) {
      [self.defaultSections addObject:[[TracksSection alloc] initWithTitle:@(block.m_blockName.c_str())
                                                                  trackIds:[BookmarksVC trackIds:block.m_trackIds]
                                                                isEditable:[self isEditable]]];
    }
  }
}

- (void)setSearchSection:(search::BookmarksSearchParams::Results const &)searchResults {
  if (self.searchSections == nil) {
    self.searchSections = [[NSMutableArray alloc] init];
  } else {
    [self.searchSections removeAllObjects];
  }

  [self.searchSections addObject:[[BookmarksSection alloc] initWithTitle:nil
                                                                 markIds:[BookmarksVC bookmarkIds:searchResults]
                                                              isEditable:[self isEditable]]];
}

- (void)refreshDefaultSections {
  if (self.defaultSections != nil)
    return;

  [self setCategorySections];
  [self updateControlsVisibility];

  auto const &bm = GetFramework().GetBookmarkManager();
  BookmarkManager::SortingType lastSortingType;
  if (bm.GetLastSortingType(self.categoryId, lastSortingType)) {
    auto const availableSortingTypes = [self availableSortingTypes];
    for (auto availableType : availableSortingTypes) {
      if (availableType == lastSortingType) {
        [self sort:lastSortingType];
        break;
      }
    }
  }
}

- (void)enableSortButton:(BOOL)enable
{
  if ([self isEditable])
    self.sortItem.enabled = enable;
  else
    self.sortDownloadedSpinnerItem.enabled = enable;
}

- (void)updateControlsVisibility {
  if ([[MWMBookmarksManager sharedManager] isCategoryNotEmpty:self.categoryId]) {
    if ([self isEditable])
      self.navigationItem.rightBarButtonItem = self.editButtonItem;
    [self enableSortButton:YES];
  } else {
    self.navigationItem.rightBarButtonItem = nil;
    [self enableSortButton:NO];
  }
}

- (void)viewDidLoad {
  [super viewDidLoad];

  self.searchBar.delegate = self;
  self.searchBar.placeholder = L(@"search_in_the_list");

  [self.noResultsView setTranslatesAutoresizingMaskIntoConstraints:NO];

  self.tableView.estimatedRowHeight = 44;
  [self.tableView registerNibWithCellClass:MWMCategoryInfoCell.class];

  auto regularTitleAttributes = @{ NSFontAttributeName: [UIFont regular16],
                                   NSForegroundColorAttributeName: [UIColor linkBlue] };
  auto moreTitleAttributes = @{ NSFontAttributeName: [UIFont medium16],
                                   NSForegroundColorAttributeName: [UIColor linkBlue] };

  [self.moreItem setTitleTextAttributes:moreTitleAttributes forState:UIControlStateNormal];
  [self.sortItem setTitleTextAttributes:regularTitleAttributes forState:UIControlStateNormal];
  [self.sortDownloadedItem setTitleTextAttributes:regularTitleAttributes forState:UIControlStateNormal];
  [self.viewOnMapItem setTitleTextAttributes:regularTitleAttributes forState:UIControlStateNormal];

  self.moreItem.title = L(@"placepage_more_button");
  self.sortItem.title = L(@"sort");
  self.sortDownloadedItem.title = L(@"sort");
  if ([self isEditable])
    self.sortSpinnerItem.customView = self.sortSpinner;
  else
    self.sortDownloadedSpinnerItem.customView = self.sortSpinner;
  
  self.viewOnMapItem.title = L(@"view_on_map_bookmarks");

  self.myCategoryToolbar.barTintColor = [UIColor white];
  self.downloadedCategoryToolbar.barTintColor = [UIColor white];

  [self refreshDefaultSections];
}

- (void)viewWillAppear:(BOOL)animated {
  [MWMLocationManager addObserver:self];

  BOOL searchAllowed = [[MWMBookmarksManager sharedManager] isCategoryNotEmpty:self.categoryId] &&
    [[MWMBookmarksManager sharedManager] isSearchAllowed:self.categoryId];
  
  if ([self isEditable]) {
    self.myCategoryToolbar.hidden = NO;
    self.downloadedCategoryToolbar.hidden = YES;
  } else {
    self.myCategoryToolbar.hidden = YES;
    self.downloadedCategoryToolbar.hidden = NO;
  }
  self.showSearchBar.priority = searchAllowed ? UILayoutPriorityRequired - 1 : UILayoutPriorityDefaultLow;
  self.hideSearchBar.priority = searchAllowed ? UILayoutPriorityDefaultLow : UILayoutPriorityRequired - 1;

  [super viewWillAppear:animated];

  auto const &bm = GetFramework().GetBookmarkManager();
  self.title = @(bm.GetCategoryName(self.categoryId).c_str());
}

- (void)viewWillDisappear:(BOOL)animated {
  [MWMLocationManager removeObserver:self];

  // Save possibly edited set name
  [super viewWillDisappear:animated];
}

- (void)viewDidAppear:(BOOL)animated {
  // Disable all notifications in BM on appearance of this view.
  // It allows to significantly improve performance in case of bookmarks
  // modification. All notifications will be sent on controller's disappearance.
  [[MWMBookmarksManager sharedManager] setNotificationsEnabled: NO];
  
  [super viewDidAppear:animated];
}

- (void)viewDidDisappear:(BOOL)animated {
  // Allow to send all notifications in BM.
  [[MWMBookmarksManager sharedManager] setNotificationsEnabled: YES];
  
  [super viewDidDisappear:animated];
}

- (IBAction)onMore:(UIBarButtonItem *)sender {
  MWMTrackIDCollection trackIds = [[MWMBookmarksManager sharedManager] trackIdsForCategory:self.categoryId];
  MWMMarkIDCollection bookmarkdsIds = [[MWMBookmarksManager sharedManager] bookmarkIdsForCategory:self.categoryId];

  auto actionSheet = [UIAlertController alertControllerWithTitle:nil
                                                         message:nil
                                                  preferredStyle:UIAlertControllerStyleActionSheet];

  if (trackIds.count > 0 || bookmarkdsIds.count > 0) {
    [actionSheet addAction:[UIAlertAction actionWithTitle:L(@"sharing_options")
                                                  style:UIAlertActionStyleDefault
                                                handler:^(UIAlertAction *_Nonnull action) {
                                                  [self shareCategory];
                                                  [Statistics logEvent:kStatBookmarksListItemSettings
                                                        withParameters:@{kStatOption: kStatSharingOptions}];
                                                }]];
  }

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
                         [[MWMBookmarksManager sharedManager] deleteCategory:self.categoryId];
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

- (IBAction)onViewOnMap:(UIBarButtonItem *)sender {
  [self viewOnMap];
}

- (void)openCategorySettings {
  auto settingsController = [[CategorySettingsViewController alloc]
                             initWithBookmarkGroup:[[MWMBookmarksManager sharedManager] categoryWithId:self.categoryId]];
  settingsController.delegate = self;
  [self.navigationController pushViewController:settingsController animated:YES];
}

- (void)exportFile {
  [[MWMBookmarksManager sharedManager] addObserver:self];
  [[MWMBookmarksManager sharedManager] shareCategory:self.categoryId];
}

- (void)shareCategory {
  auto storyboard = [UIStoryboard instance:MWMStoryboardSharing];
  auto shareController = (BookmarksSharingViewController *)[storyboard instantiateInitialViewController];
  shareController.delegate = self;
  shareController.category = [[MWMBookmarksManager sharedManager] categoryWithId:self.categoryId];
  [self.navigationController pushViewController:shareController animated:YES];
}

- (void)viewOnMap {
  [self.navigationController popToRootViewControllerAnimated:YES];
  GetFramework().ShowBookmarkCategory(self.categoryId);
}

- (IBAction)onSort:(UIBarButtonItem *)sender {
  auto actionSheet = [UIAlertController alertControllerWithTitle:nil
                                                         message:nil
                                                  preferredStyle:UIAlertControllerStyleActionSheet];

  auto const sortingTypes = [self availableSortingTypes];

  for (auto type : sortingTypes) {
    [actionSheet addAction:[UIAlertAction actionWithTitle:[BookmarksVC localizedSortingType:type]
                                                    style:UIAlertActionStyleDefault
                                                  handler:^(UIAlertAction *_Nonnull action) {
                                                    auto &bm = GetFramework().GetBookmarkManager();
                                                    bm.SetLastSortingType(self.categoryId, type);
                                                    auto const option = [BookmarksVC statisticsSortingOption:type];
                                                    [Statistics logEvent:kStatBookmarksListSort
                                                          withParameters:@{kStatOption: option}];
                                                    [self sort:type];
                                                  }]];
  }

  [actionSheet addAction:[UIAlertAction actionWithTitle:L(@"sort_default")
                                                  style:UIAlertActionStyleDefault
                                                handler:^(UIAlertAction *_Nonnull action) {
                                                  [Statistics logEvent:kStatBookmarksListSort
                                                        withParameters:@{kStatOption: kStatSortByDefault}];
                                                  [self sortDefault];
                                                }]];

  [actionSheet addAction:[UIAlertAction actionWithTitle:L(@"cancel") style:UIAlertActionStyleCancel handler:nil]];

  actionSheet.popoverPresentationController.barButtonItem = [self isEditable] ? self.sortItem
                                                                              : self.sortDownloadedItem;
  [self presentViewController:actionSheet
                     animated:YES
                   completion:^{
                     actionSheet.popoverPresentationController.passthroughViews = nil;
                   }];
}

- (std::vector<BookmarkManager::SortingType>)availableSortingTypes {
  CLLocation *lastLocation = [MWMLocationManager lastLocation];
  bool const hasMyPosition = lastLocation != nil;
  auto const &bm = GetFramework().GetBookmarkManager();
  auto const sortingTypes = bm.GetAvailableSortingTypes(self.categoryId, hasMyPosition);
  return sortingTypes;
}

- (void)sortDefault {
  auto &bm = GetFramework().GetBookmarkManager();
  bm.ResetLastSortingType(self.categoryId);
  [self setCategorySections];
  [self.tableView reloadData];
}

- (void)sort:(BookmarkManager::SortingType)type {
  bool hasMyPosition = false;
  m2::PointD myPosition = m2::PointD::Zero();

  if (type == BookmarkManager::SortingType::ByDistance) {
    CLLocation *lastLocation = [MWMLocationManager lastLocation];
    if (!lastLocation)
      return;
    hasMyPosition = true;
    myPosition = mercator::FromLatLon(lastLocation.coordinate.latitude, lastLocation.coordinate.longitude);
  }

  auto const sortId = ++self.lastSortId;
  __weak auto weakSelf = self;

  auto &bm = GetFramework().GetBookmarkManager();
  BookmarkManager::SortParams sortParams;
  sortParams.m_groupId = self.categoryId;
  sortParams.m_sortingType = type;
  sortParams.m_hasMyPosition = hasMyPosition;
  sortParams.m_myPosition = myPosition;
  sortParams.m_onResults = [weakSelf, sortId](BookmarkManager::SortedBlocksCollection &&sortedBlocks,
                                              BookmarkManager::SortParams::Status status) {
    __strong auto self = weakSelf;
    if (!self || sortId != self.lastSortId)
      return;

    [self showSortSpinner:NO];
    [self enableSortButton:YES];

    if (status == BookmarkManager::SortParams::Status::Completed) {
      [self setSortedSections:sortedBlocks];
      [self.tableView reloadData];
    }
  };

  [self showSortSpinner:YES];
  [self enableSortButton:NO];
  
  bm.GetSortedCategory(sortParams);
}

- (UIActivityIndicatorView *)sortSpinner {
  if (!_sortSpinner) {
    _sortSpinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    _sortSpinner.hidesWhenStopped = YES;
  }
  return _sortSpinner;
}

- (void)showSortSpinner:(BOOL)show {
  if (show)
    [self.sortSpinner startAnimating];
  else
    [self.sortSpinner stopAnimating];
}

- (void)cancelSearch {
  GetFramework().GetSearchAPI().CancelSearch(search::Mode::Bookmarks);

  [self showNoResultsView:NO];
  self.searchSections = nil;
  [self refreshDefaultSections];
  [self.tableView reloadData];
}

- (MWMSearchNoResults *)noResultsView {
  if (!_noResultsView) {
    _noResultsView = [MWMSearchNoResults viewWithImage:[UIImage imageNamed:@"img_search_not_found"]
                                                 title:L(@"search_not_found")
                                                  text:L(@"search_not_found_query")];
  }
  return _noResultsView;
}

- (void)showNoResultsView:(BOOL)show {
  if (!show) {
    self.tableView.hidden = NO;
    self.noResultsContainer.hidden = YES;
    [self.noResultsView removeFromSuperview];
  } else {
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

- (UIActivityIndicatorView *)spinner {
  if (!_spinner) {
    _spinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    _spinner.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
    _spinner.hidesWhenStopped = YES;
  }
  return _spinner;
}

- (NSString *)categoryFileName {
  return @(GetFramework().GetBookmarkManager().GetCategoryFileName(self.categoryId).c_str());
}

- (UIStatusBarStyle)preferredStatusBarStyle {
  return UIStatusBarStyleLightContent;
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
  [super setEditing:editing animated:animated];
  [self.tableView setEditing:editing animated:animated];
}

#pragma mark - MWMBookmarkVC

+ (MWMMarkIDCollection)bookmarkIds:(std::vector<kml::MarkId> const &)markIds {
  NSMutableArray<NSNumber *> *collection = [[NSMutableArray alloc] initWithCapacity:markIds.size()];
  for (auto const &markId : markIds) {
    [collection addObject:@(markId)];
  }
  return collection.copy;
}

+ (MWMTrackIDCollection)trackIds:(std::vector<kml::TrackId> const &)trackIds {
  NSMutableArray<NSNumber *> *collection = [[NSMutableArray alloc] initWithCapacity:trackIds.size()];
  for (auto const &trackId : trackIds) {
    [collection addObject:@(trackId)];
  }
  return collection.copy;
}

+ (NSString *)localizedSortingType:(BookmarkManager::SortingType)type {
  switch (type) {
    case BookmarkManager::SortingType::ByTime:
      return L(@"sort_date");
    case BookmarkManager::SortingType::ByDistance:
      return L(@"sort_distance");
    case BookmarkManager::SortingType::ByType:
      return L(@"sort_type");
  }
  UNREACHABLE();
}

+ (NSString *)statisticsSortingOption:(BookmarkManager::SortingType)type {
  switch (type) {
    case BookmarkManager::SortingType::ByTime:
      return kStatSortByDate;
    case BookmarkManager::SortingType::ByDistance:
      return kStatSortByDistance;
    case BookmarkManager::SortingType::ByType:
      return kStatSortByType;
  }
  UNREACHABLE();
}

#pragma mark - UISearchBarDelegate

- (BOOL)searchBarShouldBeginEditing:(UISearchBar *)searchBar {
  [self.searchBar setShowsCancelButton:YES animated:YES];
  [self.navigationController setNavigationBarHidden:YES animated:YES];
  self.tableView.contentInset = self.tableView.scrollIndicatorInsets = {};

  // Allow to send all notifications in BM.
  [[MWMBookmarksManager sharedManager] setNotificationsEnabled:YES];
  [[MWMBookmarksManager sharedManager] prepareForSearch:self.categoryId];
  
  return YES;
}

- (BOOL)searchBarShouldEndEditing:(UISearchBar *)searchBar {
  [self.searchBar setShowsCancelButton:NO animated:YES];
  [self.navigationController setNavigationBarHidden:NO animated:YES];
  self.tableView.contentInset = self.tableView.scrollIndicatorInsets = {};

  // Disable all notifications in BM.
  [[MWMBookmarksManager sharedManager] setNotificationsEnabled:NO];

  return YES;
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar {
  self.searchBar.text = @"";
  [self.searchBar resignFirstResponder];
  [self cancelSearch];
}

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText {
  if (!searchText || searchText.length == 0) {
    [self cancelSearch];
    return;
  }

  search::BookmarksSearchParams searchParams;
  searchParams.m_query = searchText.UTF8String;
  searchParams.m_groupId = self.categoryId;

  auto const searchId = ++self.lastSearchId;
  __weak auto weakSelf = self;
  searchParams.m_onStarted = [] {};
  searchParams.m_onResults = [weakSelf, searchId](search::BookmarksSearchParams::Results const &results,
                                                  search::BookmarksSearchParams::Status status) {
    __strong auto self = weakSelf;
    if (!self || searchId != self.lastSearchId)
      return;

    auto const &bm = GetFramework().GetBookmarkManager();
    auto filteredResults = results;
    bm.FilterInvalidBookmarks(filteredResults);
    [self setSearchSection:filteredResults];

    if (status == search::BookmarksSearchParams::Status::Completed) {
      [self showNoResultsView:results.empty()];
    }

    [self.tableView reloadData];
    
    [Statistics logEvent:kStatBookmarksSearch withParameters:@{kStatFrom : kStatBookmarksList}];
  };

  GetFramework().GetSearchAPI().SearchInBookmarks(searchParams);
}

#pragma mark - UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
  return [[self currentSections] count];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
  return [[self currentSections][section] numberOfRows];
}

- (nullable NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
  return [[self currentSections][section] title];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
  UITableViewCell *cell = [[self currentSections][indexPath.section] tableView:tableView cellForRow:indexPath.row];
  return cell;
}

- (nullable NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath {
  if ([[self currentSections][indexPath.section] canSelect])
    return indexPath;
  return nil;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
  // Remove cell selection
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];
  [[self currentSections][indexPath.section] didSelectRow:indexPath.row];
  if (self.searchSections != nil)
    [Statistics logEvent:kStatBookmarksSearchResultSelected withParameters:@{kStatFrom : kStatBookmarksList}];
  [self.searchBar resignFirstResponder];
  [self.navigationController popToRootViewControllerAnimated:NO];
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
  return [[self currentSections][indexPath.section] canEdit];
}

- (void)tableView:(UITableView *)tableView willBeginEditingRowAtIndexPath:(NSIndexPath *)indexPath {
  self.editing = YES;
}

- (void)tableView:(UITableView *)tableView didEndEditingRowAtIndexPath:(nullable NSIndexPath *)indexPath {
  self.editing = NO;
}

- (void)tableView:(UITableView *)tableView
  commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
   forRowAtIndexPath:(NSIndexPath *)indexPath {
  if (![[self currentSections][indexPath.section] canEdit])
    return;

  if (editingStyle == UITableViewCellEditingStyleDelete) {
    [[self currentSections][indexPath.section] deleteRow:indexPath.row];
    // In the case of search section editing reset cached default sections.
    if (self.searchSections != nil)
      self.defaultSections = nil;
    [self updateControlsVisibility];
  }

  if ([[self currentSections][indexPath.section] numberOfRows] == 0) {
    [[self currentSections] removeObjectAtIndex:indexPath.section];
    auto indexSet = [NSIndexSet indexSetWithIndex:indexPath.section];
    [self.tableView deleteSections:indexSet withRowAnimation:UITableViewRowAnimationFade];
  } else {
    [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
  }

  auto const &bm = GetFramework().GetBookmarkManager();
  if (bm.GetUserMarkIds(self.categoryId).size() + bm.GetTrackIds(self.categoryId).size() == 0) {
    self.navigationItem.rightBarButtonItem = nil;
    [self setEditing:NO animated:YES];
  }
}

#pragma mark - InfoSectionDelegate

- (void)infoSectionUpdates:(void (^)(void))updates {
  [self.tableView update:updates];
}

#pragma mark - MWMLocationObserver

- (void)onLocationUpdate:(CLLocation *)location {
  [self.tableView.visibleCells enumerateObjectsUsingBlock:^(UITableViewCell *cell, NSUInteger idx, BOOL *stop) {
    auto const indexPath = [self.tableView indexPathForCell:cell];
    auto const &section = [self currentSections][indexPath.section];
    if ([section respondsToSelector:@selector(updateCell:forRow:withNewLocation:)])
      [section updateCell:cell forRow:indexPath.row withNewLocation:location];
  }];
}

#pragma mark - MWMBookmarksObserver

- (void)onBookmarksCategoryFilePrepared:(MWMBookmarksShareStatus)status {
  switch (status) {
    case MWMBookmarksShareStatusSuccess: {
      auto shareController = [MWMActivityViewController
        shareControllerForURL:[MWMBookmarksManager sharedManager].shareCategoryURL
                      message:L(@"share_bookmarks_email_body")
            completionHandler:^(UIActivityType _Nullable activityType, BOOL completed, NSArray *_Nullable returnedItems,
                                NSError *_Nullable activityError) {
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

- (void)didShareCategory {
  [self.tableView reloadData];
}

#pragma mark - CategorySettingsViewControllerDelegate

- (void)categorySettingsController:(CategorySettingsViewController *)viewController
                         didDelete:(MWMMarkGroupID)categoryId {
  [self.delegate bookmarksVCdidDeleteCategory:self];
}

- (void)categorySettingsController:(CategorySettingsViewController *)viewController
                     didEndEditing:(MWMMarkGroupID)categoryId {
  [self.navigationController popViewControllerAnimated:YES];
  [self.delegate bookmarksVCdidUpdateCategory:self];
  self.infoSection = nil;
  self.defaultSections = nil;
  [self refreshDefaultSections];
  [self.tableView reloadData];
}

#pragma mark - MWMKeyboard

- (void)onKeyboardAnimation {
  self.noResultsBottom.constant = 0;
  CGFloat const keyboardHeight = [MWMKeyboard keyboardHeight];
  if (keyboardHeight >= self.noResultsContainer.height)
    return;

  self.noResultsBottom.constant = -keyboardHeight;
}

#pragma mark - UIScrollViewDelegate

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView
{
  [self.searchBar resignFirstResponder];
}

@end
