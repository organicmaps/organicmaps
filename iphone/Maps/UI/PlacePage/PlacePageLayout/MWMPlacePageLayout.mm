#import "MWMPlacePageLayout.h"
#import "MWMBookmarkCell.h"
#import "MWMCircularProgress.h"
#import "MWMiPadPlacePageLayoutImpl.h"
#import "MWMiPhonePlacePageLayoutImpl.h"
#import "MWMOpeningHoursLayoutHelper.h"
#import "MWMPlacePageButtonCell.h"
#import "MWMPlacePageCellUpdateProtocol.h"
#import "MWMPlacePageData.h"
#import "MWMPlacePageInfoCell.h"
#import "MWMPlacePageLayoutImpl.h"
#import "MWMPlacePageTaxiCell.h"
#import "MWMPPPreviewLayoutHelper.h"
#import "UIColor+MapsMeColor.h"

#include "storage/storage.hpp"

#include "std/array.hpp"
#include "std/map.hpp"

namespace
{
array<NSString *, 1> const kBookmarkCells = {{@"MWMBookmarkCell"}};

using place_page::MetainfoRows;

map<MetainfoRows, NSString *> const kMetaInfoCells = {
  {MetainfoRows::Website, @"PlacePageLinkCell"},
  {MetainfoRows::Address, @"PlacePageInfoCell"},
  {MetainfoRows::Email, @"PlacePageLinkCell"},
  {MetainfoRows::Phone, @"PlacePageLinkCell"},
  {MetainfoRows::Cuisine, @"PlacePageInfoCell"},
  {MetainfoRows::Operator, @"PlacePageInfoCell"},
  {MetainfoRows::Coordinate, @"PlacePageInfoCell"},
  {MetainfoRows::Internet, @"PlacePageInfoCell"},
  {MetainfoRows::Taxi, @"MWMPlacePageTaxiCell"}};

array<NSString *, 1> const kButtonsCells = {{@"MWMPlacePageButtonCell"}};

}  // namespace

@interface MWMPlacePageLayout () <UITableViewDataSource,
                                 MWMPlacePageCellUpdateProtocol, MWMPlacePageViewUpdateProtocol>

@property(weak, nonatomic) MWMPlacePageData * data;

@property(weak, nonatomic) UIView * ownerView;
@property(weak, nonatomic)
    id<MWMPlacePageLayoutDelegate, MWMPlacePageButtonsProtocol, MWMActionBarProtocol>
        delegate;
@property(weak, nonatomic) id<MWMPlacePageLayoutDataSource> dataSource;
@property(nonatomic) IBOutlet MWMPPView * placePageView;

@property(nonatomic) MWMBookmarkCell * bookmarkCell;

@property(nonatomic) MWMPlacePageActionBar * actionBar;

@property(nonatomic) BOOL isPlacePageButtonsEnabled;
@property(nonatomic) BOOL isDownloaderViewShown;

@property(nonatomic) id<MWMPlacePageLayoutImpl> layoutImpl;

@property(nonatomic) MWMPPPreviewLayoutHelper * previewLayoutHelper;
@property(nonatomic) MWMOpeningHoursLayoutHelper * openingHoursLayoutHelper;

@end

@implementation MWMPlacePageLayout

- (instancetype)initWithOwnerView:(UIView *)view
                         delegate:(id<MWMPlacePageLayoutDelegate, MWMPlacePageButtonsProtocol,
                                      MWMActionBarProtocol>)delegate
                       dataSource:(id<MWMPlacePageLayoutDataSource>)dataSource
{
  self = [super init];
  if (self)
  {
    _ownerView = view;
    _delegate = delegate;
    _dataSource = dataSource;
    [[NSBundle mainBundle] loadNibNamed:[MWMPPView className] owner:self options:nil];
    [_placePageView layoutIfNeeded];
    _placePageView.delegate = self;
    auto const Impl = IPAD ? [MWMiPadPlacePageLayoutImpl class] : [MWMiPhonePlacePageLayoutImpl class];
    _layoutImpl = [[Impl alloc] initOwnerView:view placePageView:_placePageView delegate:delegate];

    if ([_layoutImpl respondsToSelector:@selector(setInitialTopBound:leftBound:)])
      [_layoutImpl setInitialTopBound:dataSource.topBound leftBound:dataSource.leftBound];

    auto tableView = _placePageView.tableView;
    _previewLayoutHelper = [[MWMPPPreviewLayoutHelper alloc]
                                                     initWithTableView:tableView];
    _openingHoursLayoutHelper = [[MWMOpeningHoursLayoutHelper alloc] initWithTableView:tableView];
    [self registerCells];
  }
  return self;
}

- (void)registerCells
{
  auto tv = self.placePageView.tableView;

  [tv registerNib:[UINib nibWithNibName:kButtonsCells[0] bundle:nil]
                  forCellReuseIdentifier:kButtonsCells[0]];
  [tv registerNib:[UINib nibWithNibName:kBookmarkCells[0] bundle:nil]
                  forCellReuseIdentifier:kBookmarkCells[0]];

  // Register all meta info cells.
  for (auto const & pair : kMetaInfoCells)
  {
    NSString * name = pair.second;
    [tv registerNib:[UINib nibWithNibName:name bundle:nil] forCellReuseIdentifier:name];
  }
}

- (void)layoutWithSize:(CGSize const &)size
{
  [self.layoutImpl onScreenResize:size];
}

- (UIView *)shareAnchor { return self.actionBar.shareAnchor; }
- (void)showWithData:(MWMPlacePageData *)data
{
  self.isPlacePageButtonsEnabled = YES;
  self.data = data;
  self.bookmarkCell = nil;

  [self.actionBar configureWithData:static_cast<id<MWMActionBarSharedData>>(data)];
  [self.previewLayoutHelper configWithData:data];
  [self.openingHoursLayoutHelper configWithData:data];
  if ([self.layoutImpl respondsToSelector:@selector(setPreviewLayoutHelper:)])
    [self.layoutImpl setPreviewLayoutHelper:self.previewLayoutHelper];

  [self.placePageView.tableView reloadData];
  [self.layoutImpl onShow];
}

- (void)rotateDirectionArrowToAngle:(CGFloat)angle
{
  [self.previewLayoutHelper rotateDirectionArrowToAngle:angle];
}

- (void)setDistanceToObject:(NSString *)distance
{
  [self.previewLayoutHelper setDistanceToObject:distance];
}

- (MWMPlacePageActionBar *)actionBar
{
  if (!_actionBar)
  {
    _actionBar = [MWMPlacePageActionBar actionBarWithDelegate:self.delegate];
    self.layoutImpl.actionBar = _actionBar;
  }
  return _actionBar;
}

- (void)close
{
  [self.layoutImpl onClose];
}

- (void)mwm_refreshUI
{
  [self.placePageView mwm_refreshUI];
  [self.actionBar mwm_refreshUI];
}

- (void)reloadBookmarkSection:(BOOL)isBookmark
{
  auto tv = self.placePageView.tableView;
  NSIndexSet * set =
      [NSIndexSet indexSetWithIndex:static_cast<NSInteger>(place_page::Sections::Bookmark)];

  if (isBookmark)
  {
    if (self.bookmarkCell)
      [tv reloadSections:set withRowAnimation:UITableViewRowAnimationAutomatic];
    else
      [tv insertSections:set withRowAnimation:UITableViewRowAnimationAutomatic];
  }
  else
  {
    [tv deleteSections:set withRowAnimation:UITableViewRowAnimationAutomatic];
    self.bookmarkCell = nil;
  }
}

#pragma mark - Downloader event

- (void)processDownloaderEventWithStatus:(storage::NodeStatus)status progress:(CGFloat)progress
{
  using namespace storage;

  auto const & sections = self.data.sections;
  switch (status)
  {
  case NodeStatus::OnDiskOutOfDate:
  case NodeStatus::Undefined:
  {
    self.isPlacePageButtonsEnabled = NO;
    auto const it = find(sections.begin(), sections.end(), place_page::Sections::Buttons);
    if (it != sections.end())
    {
      [self.placePageView.tableView
            reloadSections:[NSIndexSet indexSetWithIndex:distance(sections.begin(), it)]
          withRowAnimation:UITableViewRowAnimationAutomatic];
    }
    self.actionBar.isAreaNotDownloaded = NO;
    break;
  }
  case NodeStatus::Downloading:
  {
    self.actionBar.isAreaNotDownloaded = YES;
    self.actionBar.downloadingProgress = progress;
    break;
  }
  case NodeStatus::InQueue:
  {
    self.actionBar.isAreaNotDownloaded = YES;
    self.actionBar.downloadingState = MWMCircularProgressStateSpinner;
    break;
  }
  case NodeStatus::Error:
  {
    self.actionBar.isAreaNotDownloaded = YES;
    self.actionBar.downloadingState = MWMCircularProgressStateFailed;
    break;
  }
  case NodeStatus::Partly: break;
  case NodeStatus::OnDisk:
  {
    self.isPlacePageButtonsEnabled = YES;
    auto const it = find(sections.begin(), sections.end(), place_page::Sections::Buttons);
    if (it != sections.end())
    {
      [self.placePageView.tableView
            reloadSections:[NSIndexSet indexSetWithIndex:distance(sections.begin(), it)]
          withRowAnimation:UITableViewRowAnimationAutomatic];
    }
    self.actionBar.isAreaNotDownloaded = NO;
    break;
  }
  case NodeStatus::NotDownloaded:
  {
    self.isPlacePageButtonsEnabled = NO;
    self.actionBar.isAreaNotDownloaded = YES;
    self.actionBar.downloadingState = MWMCircularProgressStateNormal;
    break;
  }
  }
}

#pragma mark - iPad only

- (void)updateTopBound
{
  if (![self.layoutImpl respondsToSelector:@selector(updateLayoutWithTopBound:)])
  {
    NSAssert(!IPAD, @"iPad layout must implement updateLayoutWithTopBound:!");
    return;
  }

  [self.layoutImpl updateLayoutWithTopBound:self.dataSource.topBound];
}

- (void)updateLeftBound
{
  if (![self.layoutImpl respondsToSelector:@selector(updateLayoutWithLeftBound:)])
  {
    NSAssert(!IPAD, @"iPad layout must implement updateLayoutWithLeftBound:!");
    return;
  }

  [self.layoutImpl updateLayoutWithLeftBound:self.dataSource.leftBound];
}

#pragma mark - UITableViewDelegate & UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  auto data = self.data;
  if (!data)
    return 0;
  return data.sections.size();
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  using namespace place_page;

  auto data = self.data;
  switch (data.sections[section])
  {
  case Sections::Bookmark: return 1;
  case Sections::Preview: return data.previewRows.size();
  case Sections::Metainfo: return data.metainfoRows.size();
  case Sections::Buttons: return data.buttonsRows.size();
  }
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  using namespace place_page;

  auto data = self.data;
  id<MWMPlacePageButtonsProtocol> delegate = self.delegate;
  switch (data.sections[indexPath.section])
  {
  case Sections::Preview:
  {
    return [self.previewLayoutHelper cellForRowAtIndexPath:indexPath withData:data];
  }
  case Sections::Bookmark:
  {
    MWMBookmarkCell * c = [tableView dequeueReusableCellWithIdentifier:kBookmarkCells[0]];
    [c configureWithText:data.bookmarkDescription
          updateCellDelegate:self
        editBookmarkDelegate:delegate
                      isHTML:data.isHTMLDescription];
    return c;
  }
  case Sections::Metainfo:
  {
    auto const row = data.metainfoRows[indexPath.row];
    switch (row)
    {
    case MetainfoRows::OpeningHours:
    case MetainfoRows::ExtendedOpeningHours:
      return [self.openingHoursLayoutHelper cellForRowAtIndexPath:indexPath];
    case MetainfoRows::Phone:
    case MetainfoRows::Address:
    case MetainfoRows::Website:
    case MetainfoRows::Email:
    case MetainfoRows::Cuisine:
    case MetainfoRows::Operator:
    case MetainfoRows::Internet:
    case MetainfoRows::Coordinate:
    {
      MWMPlacePageInfoCell * c = [tableView dequeueReusableCellWithIdentifier:kMetaInfoCells.at(row)];
      [c configWithRow:row data:data];
      return c;
    }
    case MetainfoRows::Taxi:
    {
      MWMPlacePageTaxiCell * c = [tableView dequeueReusableCellWithIdentifier:kMetaInfoCells.at(row)];
      c.delegate = delegate;
      return c;
    }
    }
  }
  case Sections::Buttons:
  {
    MWMPlacePageButtonCell * c = [tableView dequeueReusableCellWithIdentifier:kButtonsCells[0]];
    auto const row = data.buttonsRows[indexPath.row];
    [c configForRow:row withDelegate:delegate];

    if (row != ButtonsRows::HotelDescription)
      [c setEnabled:self.isPlacePageButtonsEnabled];
    else
      [c setEnabled:YES];

    return c;
  }
  }
}

#pragma mark - MWMPlacePageCellUpdateProtocol

- (void)cellUpdated
{
  auto const update = @selector(update);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:update object:nil];
  [self performSelector:update withObject:nil afterDelay:0.1];
}

- (void)update
{
  auto tableView = self.placePageView.tableView;
  [tableView beginUpdates];
  [tableView endUpdates];
}

#pragma mark - MWMPlacePageViewUpdateProtocol

- (void)updateWithHeight:(CGFloat)height
{
  auto const sel = @selector(updatePlacePageHeight);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:sel object:nil];
  [self performSelector:sel withObject:nil afterDelay:0.1];
}

- (void)updatePlacePageHeight
{
  [self.layoutImpl onUpdatePlacePageWithHeight:self.placePageView.tableView.contentSize.height];
}

@end
