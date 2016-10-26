#import "MWMPlacePageLayout.h"
#import "MWMCircularProgress.h"
#import "MWMPPView.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageCellUpdateProtocol.h"
#import "MWMPlacePageData.h"
#import "MWMBookmarkCell.h"
#import "MWMOpeningHoursCell.h"
#import "MWMPlacePageBookmarkCell.h"
#import "MWMPlacePageButtonCell.h"
#import "MWMPlacePageInfoCell.h"
#import "MWMPlacePagePreviewCell.h"
#import "UIColor+MapsMeColor.h"

#include "storage/storage.hpp"

#include "std/array.hpp"

namespace
{
enum class ScrollDirection
{
  Up,
  Down
};

enum class State
{
  Bottom,
  Top
};

// Minimal offset for collapse. If place page offset is below this value we should hide place page.
CGFloat const kMinOffset = 1;
CGFloat const kOpenPlacePageStopValue = 0.7;
CGFloat const kLuftDraggingOffset = 30;

array<NSString *, 1> kPreviewCells = {{@"MWMPlacePagePreviewCell"}};

array<NSString *, 1> kBookmarkCells = {{@"MWMBookmarkCell"}};

array<NSString *, 9> kMetaInfoCells = {
    {@"MWMOpeningHoursCell", @"PlacePageLinkCell", @"PlacePageInfoCell", @"PlacePageLinkCell",
     @"PlacePageLinkCell", @"PlacePageInfoCell", @"PlacePageInfoCell", @"PlacePageInfoCell",
     @"PlacePageInfoCell"}};

array<NSString *, 1> kButtonsCells = {{@"MWMPlacePageButtonCell"}};

NSTimeInterval const kAnimationDuration = 0.25;

void animate(TMWMVoidBlock animate, TMWMVoidBlock completion = nil)
{
  [UIView animateWithDuration:kAnimationDuration
      delay:0
      options:UIViewAnimationOptionCurveEaseIn
      animations:^{
        animate();
      }
      completion:^(BOOL finished) {
        if (completion)
          completion();
      }];
}

}  // namespace

@interface MWMPlacePageLayout ()<UITableViewDelegate, UITableViewDataSource,
                                 MWMPlacePageCellUpdateProtocol, MWMPlacePageViewUpdateProtocol>

@property(weak, nonatomic) MWMPlacePageData * data;

@property(weak, nonatomic) UIView * ownerView;
@property(weak, nonatomic)
    id<MWMPlacePageLayoutDelegate, MWMPlacePageButtonsProtocol, MWMActionBarProtocol>
        delegate;
@property(weak, nonatomic) id<MWMPlacePageLayoutDataSource> dataSource;

@property(nonatomic) MWMPPScrollView * scrollView;
@property(nonatomic) IBOutlet MWMPPView * placePageView;

@property(nonatomic) ScrollDirection direction;
@property(nonatomic) State state;

@property(nonatomic) CGFloat portraitOpenContentOffset;
@property(nonatomic) CGFloat landscapeOpenContentOffset;
@property(nonatomic) CGFloat lastContentOffset;
@property(nonatomic) CGFloat expandedContentOffset;

@property(nonatomic) MWMPlacePagePreviewCell * ppPreviewCell;
@property(nonatomic) MWMBookmarkCell * bookmarkCell;

@property(nonatomic) MWMPlacePageActionBar * actionBar;

@property(nonatomic) BOOL isPlacePageButtonsEnabled;
@property(nonatomic) BOOL isDownloaderViewShown;

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
    auto view = self.ownerView;
    auto const & size = view.size;
    _placePageView.frame = {{0, size.height}, size};
    _placePageView.delegate = self;
    _scrollView = [[MWMPPScrollView alloc] initWithFrame:view.frame inactiveView:_placePageView];
    _portraitOpenContentOffset = MAX(size.width, size.height) * kOpenPlacePageStopValue;
    _landscapeOpenContentOffset = MIN(size.width, size.height) * kOpenPlacePageStopValue;
    [view addSubview:_scrollView];
    [_scrollView addSubview:_placePageView];
    [self registerCells];
  }
  return self;
}

- (void)registerCells
{
  auto tv = self.placePageView.tableView;

  [tv registerNib:[UINib nibWithNibName:kPreviewCells[0] bundle:nil]
                  forCellReuseIdentifier:kPreviewCells[0]];
  [tv registerNib:[UINib nibWithNibName:kButtonsCells[0] bundle:nil]
                  forCellReuseIdentifier:kButtonsCells[0]];
  [tv registerNib:[UINib nibWithNibName:kBookmarkCells[0] bundle:nil]
                  forCellReuseIdentifier:kBookmarkCells[0]];

  // Register all meta info cells.
  for (auto const name : kMetaInfoCells)
    [tv registerNib:[UINib nibWithNibName:name bundle:nil] forCellReuseIdentifier:name];

}

- (void)layoutWithSize:(CGSize const &)size
{
  self.scrollView.frame = {{}, size};
  self.placePageView.origin = {0., size.height};
  self.actionBar.frame = {{0., size.height - self.actionBar.height},
                          {size.width, self.actionBar.height}};
  [self.delegate onTopBoundChanged:self.scrollView.contentOffset.y];
}

- (UIView *)shareAnchor { return self.actionBar.shareAnchor; }
- (void)showWithData:(MWMPlacePageData *)data
{
  self.isPlacePageButtonsEnabled = YES;
  self.data = data;
  self.ppPreviewCell = nil;
  self.bookmarkCell = nil;

  self.scrollView.delegate = self;
  self.state = State::Bottom;

  [self collapse];

  [self.actionBar configureWithData:static_cast<id<MWMActionBarSharedData>>(data)];
  [self.placePageView.tableView reloadData];
}

- (void)rotateDirectionArrowToAngle:(CGFloat)angle
{
  [self.ppPreviewCell rotateDirectionArrowToAngle:angle];
}

- (void)setDistanceToObject:(NSString *)distance
{
  [self.ppPreviewCell setDistanceToObject:distance];
}

- (MWMPlacePageActionBar *)actionBar
{
  if (!_actionBar)
  {
    _actionBar = [MWMPlacePageActionBar actionBarWithDelegate:self.delegate];
    UIView * superview = self.ownerView;
    _actionBar.origin = {0., superview.height};
    [superview addSubview:_actionBar];
  }
  return _actionBar;
}

- (void)close
{
  animate(
  ^{
     self.actionBar.origin = {0., self.ownerView.height};
     [self.scrollView setContentOffset:{} animated:YES];
   },
   ^{
     [self.actionBar removeFromSuperview];
     self.actionBar = nil;
     [self.delegate shouldDestroyLayout];
   });
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

- (void)collapse
{;
  self.scrollView.scrollEnabled = NO;
  [self.placePageView hideTableView:YES];

  animate(^{
    [self.scrollView setContentOffset:{ 0., kMinOffset } animated:YES];
  });
}

- (void)expand
{
  self.actionBar.hidden = NO;
  self.scrollView.scrollEnabled = YES;

  animate(^{
    [self.placePageView hideTableView:NO];
    self.actionBar.minY = self.actionBar.superview.height - self.actionBar.height;

    // We decrease expanded offset for 2 pixels because it looks more clear.
    auto constexpr designOffset = 2;
    self.expandedContentOffset =
        self.ppPreviewCell.height + self.placePageView.top.height + self.actionBar.height - designOffset;

    auto const targetOffset =
        self.state == State::Bottom ? self.expandedContentOffset : self.topContentOffset;
    [self.scrollView setContentOffset:{ 0, targetOffset } animated:YES];
  });
}

- (BOOL)isPortrait
{
  auto const & s = self.ownerView.size;
  return s.height > s.width;
}

- (CGFloat)openContentOffset
{
  return self.isPortrait ? self.portraitOpenContentOffset : self.landscapeOpenContentOffset;
}

- (CGFloat)topContentOffset
{
  auto const target = self.openContentOffset;
  if (target > self.placePageView.height)
    return self.placePageView.height;

  return target;
}

#pragma mark - Downloader event

- (void)processDownloaderEventWithStatus:(storage::NodeStatus)status progress:(CGFloat)progress
{
  using namespace storage;

  auto const & sections = self.data.sections;
  switch (status)
  {
  case NodeStatus::Undefined:
  {
    self.isPlacePageButtonsEnabled = YES;
    auto const it = find(sections.begin(), sections.end(), place_page::Sections::Buttons);
    if (it != sections.end())
    {
      [self.placePageView.tableView
            reloadSections:[NSIndexSet indexSetWithIndex:distance(sections.begin(), it)]
          withRowAnimation:UITableViewRowAnimationAutomatic];
    }

    if (self.ppPreviewCell)
      [self.ppPreviewCell setDownloaderViewHidden:YES animated:NO];
    else
      self.isDownloaderViewShown = NO;

    break;
  }
  case NodeStatus::Downloading:
  {
    self.ppPreviewCell.mapDownloadProgress.progress = progress;
    break;
  }
  case NodeStatus::InQueue:
  {
    self.ppPreviewCell.mapDownloadProgress.state = MWMCircularProgressStateSpinner;
    break;
  }
  case NodeStatus::Error:
  {
    self.ppPreviewCell.mapDownloadProgress.state = MWMCircularProgressStateFailed;
    break;
  }
  case NodeStatus::Partly: break;
  case NodeStatus::OnDiskOutOfDate:
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
    [self.ppPreviewCell setDownloaderViewHidden:YES animated:NO];
    break;
  }
  case NodeStatus::NotDownloaded:
  {
    self.isPlacePageButtonsEnabled = NO;
    if (self.ppPreviewCell)
      [self.ppPreviewCell setDownloaderViewHidden:NO animated:NO];
    else
      self.isDownloaderViewShown = YES;

    break;
  }
  }
}

#pragma mark - UIScrollViewDelegate

- (void)scrollViewDidScroll:(MWMPPScrollView *)scrollView
{
  if ([scrollView isEqual:self.placePageView.tableView])
    return;

  auto const & offset = scrollView.contentOffset;
  id<MWMPlacePageLayoutDelegate> delegate = self.delegate;
  if (offset.y <= 0)
  {
    [self.scrollView removeFromSuperview];
    [self.actionBar removeFromSuperview];
    [delegate shouldDestroyLayout];
    return;
  }

  if (offset.y > self.placePageView.height + kLuftDraggingOffset)
  {
    auto const bounded = self.placePageView.height + kLuftDraggingOffset;
    [scrollView setContentOffset:{0, bounded}];
    [delegate onTopBoundChanged:bounded];
  }
  else
  {
    [delegate onTopBoundChanged:offset.y];
  }

  self.direction = self.lastContentOffset < offset.y ? ScrollDirection::Up : ScrollDirection::Down;
  self.lastContentOffset = offset.y;
}

- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView
                     withVelocity:(CGPoint)velocity
              targetContentOffset:(inout CGPoint *)targetContentOffset
{
  auto const actualOffset = scrollView.contentOffset.y;
  auto const openOffset = self.openContentOffset;
  auto const targetOffset = (*targetContentOffset).y;

  if (actualOffset > self.expandedContentOffset && actualOffset < openOffset)
  {
    auto const isDirectionUp = self.direction == ScrollDirection::Up;
    self.state = isDirectionUp ? State::Top : State::Bottom;
    (*targetContentOffset).y =
        isDirectionUp ? openOffset : self.expandedContentOffset;
  }
  else if (actualOffset > openOffset && targetOffset < openOffset)
  {
    self.state = State::Top;
    (*targetContentOffset).y = openOffset;
  }
  else if (actualOffset < self.expandedContentOffset)
  {
    (*targetContentOffset).y = 0;
    animate(^{
      self.actionBar.origin = {0., self.ownerView.height};
    });
  }
  else
  {
    self.state = State::Top;
  }
}

- (void)scrollViewDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate
{
  if (decelerate)
    return;

  auto const actualOffset = scrollView.contentOffset.y;
  auto const openOffset = self.openContentOffset;

  if (actualOffset < self.expandedContentOffset + kLuftDraggingOffset)
  {
    self.state = State::Bottom;
    animate(^{
      [scrollView setContentOffset:{ 0, self.expandedContentOffset } animated:YES];
    });
  }
  else if (actualOffset < openOffset)
  {
    auto const isDirectionUp = self.direction == ScrollDirection::Up;
    self.state = isDirectionUp ? State::Top : State::Bottom;
    animate(^{
      [scrollView setContentOffset:{0, isDirectionUp ? openOffset : self.expandedContentOffset}
                          animated:YES];
    });
  }
  else
  {
    self.state = State::Top;
  }
}

- (void)setState:(State)state
{
  _state = state;
  self.placePageView.anchorImage.transform = state == State::Top ? CGAffineTransformMakeRotation(M_PI)
                                                                 : CGAffineTransformIdentity;
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
  case Sections::Preview:
  case Sections::Bookmark: return 1;
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
    if (!self.ppPreviewCell)
      self.ppPreviewCell =
          [tableView dequeueReusableCellWithIdentifier:[MWMPlacePagePreviewCell className]];

    [self.ppPreviewCell configure:data updateLayoutDelegate:self dataSource:self.dataSource tapAction:^
    {
      CGFloat offset = 0;
      if (self.state == State::Top)
      {
        self.state = State::Bottom;
        offset = self.expandedContentOffset;
      }
      else
      {
        self.state = State::Top;
        offset = self.topContentOffset;
      }
      animate(^{ [self.scrollView setContentOffset:{0, offset} animated:YES]; });
    }];

    [self.ppPreviewCell setDownloaderViewHidden:!self.isDownloaderViewShown animated:NO];

    return self.ppPreviewCell;
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
    auto cellName = kMetaInfoCells[static_cast<size_t>(row)];
    UITableViewCell * c = [tableView dequeueReusableCellWithIdentifier:cellName];

    switch (row)
    {
    case MetainfoRows::OpeningHours:
    {
      [static_cast<MWMOpeningHoursCell *>(c)
          configureWithOpeningHours:[data stringForRow:row]
               updateLayoutDelegate:self
                        isClosedNow:data.schedule == OpeningHours::Closed];
      break;
    }
    case MetainfoRows::Phone:
    case MetainfoRows::Address:
    case MetainfoRows::Website:
    case MetainfoRows::Email:
    case MetainfoRows::Cuisine:
    case MetainfoRows::Operator:
    case MetainfoRows::Internet:
    case MetainfoRows::Coordinate:
    {
      [static_cast<MWMPlacePageInfoCell *>(c) configWithRow:row data:data];
      break;
    }
    }
    return c;
  }
  case Sections::Buttons:
  {
    MWMPlacePageButtonCell * c = [tableView dequeueReusableCellWithIdentifier:kButtonsCells[0]];
    auto const row = data.buttonsRows[indexPath.row];
    [c configForRow:row withDelegate:delegate];
    if (row != ButtonsRows::HotelDescription)
      [c setEnabled:self.isPlacePageButtonsEnabled];

    return c;
  }
  }
}

#pragma mark - MWMPlacePageCellUpdateProtocol

- (void)updateCellWithForceReposition:(BOOL)isForceReposition
{
  auto const update = isForceReposition ? @selector(updateWithExpand) : @selector(update);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:update object:nil];
  [self performSelector:update withObject:nil afterDelay:0.1];
}

- (void)update
{
  auto tableView = self.placePageView.tableView;
  [tableView beginUpdates];
  [tableView endUpdates];
}

- (void)updateWithExpand
{
  [self update];
  [self expand];
}

#pragma mark - MWMPlacePageViewUpdateProtocol

- (void)updateWithHeight:(CGFloat)height
{
  auto const & size = self.ownerView.size;
  self.scrollView.contentSize = {size.width, size.height + height};
}

@end
