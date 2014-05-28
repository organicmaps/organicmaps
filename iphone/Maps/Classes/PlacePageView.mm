

#import "PlacePageView.h"
#import "UIKitCategories.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "BookmarksRootVC.h"
#import "Framework.h"
#import "ContextViews.h"
#import "PlacePageInfoCell.h"
#import "PlacePageEditCell.h"
#import "PlacePageShareCell.h"
#include "../../search/result.hpp"

typedef NS_ENUM(NSUInteger, CellRow)
{
  CellRowCommon,
  CellRowSet,
  CellRowInfo,
  CellRowShare,
};

@interface PlacePageView () <UIGestureRecognizerDelegate, UITableViewDataSource, UITableViewDelegate, LocationObserver, PlacePageShareCellDelegate, PlacePageInfoCellDelegate>

@property (nonatomic) UIView * backgroundView;
@property (nonatomic) UIView * wrapView;
@property (nonatomic) UIView * headerView;
@property (nonatomic) UIView * headerSeparator;
@property (nonatomic) UITableView * tableView;
@property (nonatomic) CopyLabel * titleLabel;
@property (nonatomic) UIButton * bookmarkButton;
@property (nonatomic) UILabel * typeLabel;
@property (nonatomic) UIButton * shareButton;
@property (nonatomic) CALayer * maskLayer1;
@property (nonatomic) CALayer * maskLayer2;
@property (nonatomic) UIImageView * editImageView;
@property (nonatomic) UIImageView * arrowImageView;

@property (nonatomic) NSString * title;
@property (nonatomic) NSString * types;
@property (nonatomic) NSString * address;
@property (nonatomic) NSString * info;
@property (nonatomic) NSString * setName;

@end

@implementation PlacePageView
{
  UserMark const * m_mark;
  UserMark const * m_cached_mark;
  BookmarkData * m_bookmarkData;
  size_t m_categoryIndex;
}

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  self.clipsToBounds = YES;

  m_mark = NULL;
  m_cached_mark = NULL;
  m_bookmarkData = NULL;

  self.statusBarIncluded = !SYSTEM_VERSION_IS_LESS_THAN(@"7");

  [self addSubview:self.backgroundView];

  [self addSubview:self.arrowImageView];

  UISwipeGestureRecognizer * swipeUp = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipe:)];
  swipeUp.direction = UISwipeGestureRecognizerDirectionUp;
  [self addGestureRecognizer:swipeUp];

  UISwipeGestureRecognizer * swipeDown = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipe:)];
  swipeDown.direction = UISwipeGestureRecognizerDirectionDown;
  [self addGestureRecognizer:swipeDown];

  [self addSubview:self.wrapView];
  [self.wrapView addSubview:self.tableView];

  [self addSubview:self.headerView];

  NSNotificationCenter * nc = [NSNotificationCenter defaultCenter];
  [nc addObserver:self selector:@selector(bookmarkDeletedNotification:) name:BOOKMARK_DELETED_NOTIFICATION object:nil];
  [nc addObserver:self selector:@selector(bookmarkCategoryDeletedNotification:) name:BOOKMARK_CATEGORY_DELETED_NOTIFICATION object:nil];
  [nc addObserver:self selector:@selector(metricsChangedNotification:) name:METRICS_CHANGED_NOTIFICATION object:nil];

  [self.tableView registerClass:[PlacePageInfoCell class] forCellReuseIdentifier:[PlacePageInfoCell className]];
  [self.tableView registerClass:[PlacePageEditCell class] forCellReuseIdentifier:[PlacePageEditCell className]];
  [self.tableView registerClass:[PlacePageShareCell class] forCellReuseIdentifier:[PlacePageShareCell className]];

  [[MapsAppDelegate theApp].m_locationManager start:self];

  return self;
}

- (void)onLocationError:(location::TLocationError)errorCode
{
  NSLog(@"Location error %i in %@", errorCode, [[self class] className]);
}

#define ROW_COMMON 0
#define ROW_SET 1
#define ROW_INFO 2

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  static BOOL available = NO;
  if (!available)
  {
    available = YES;
    [self.tableView reloadRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:ROW_COMMON inSection:0]] withRowAnimation:UITableViewRowAnimationFade];
    [self alignAnimated:YES];
  }
}

- (CellRow)cellRowForIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.row == ROW_COMMON)
    return CellRowCommon;
  else if (indexPath.row == ROW_SET)
    return [self isBookmark] ? CellRowSet : CellRowShare;
  else if (indexPath.row == ROW_INFO)
    return CellRowInfo;
  else if (indexPath.row == 3)
    return CellRowShare;
  return 0;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self rowsCount];
}

- (NSInteger)rowsCount
{
  return [self isBookmark] ? 4 : 2;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  CellRow row = [self cellRowForIndexPath:indexPath];
  if (row == CellRowCommon)
  {
    PlacePageInfoCell * cell = [tableView dequeueReusableCellWithIdentifier:[PlacePageInfoCell className]];
    [cell setAddress:[self address] pinPoint:[self pinPoint]];
    cell.delegate = self;
    return cell;
  }
  else if (row == CellRowSet)
  {
    PlacePageEditCell * cell = [tableView dequeueReusableCellWithIdentifier:[PlacePageEditCell className]];
    cell.titleLabel.text = [self setName];
    return cell;
  }
  else if (row == CellRowInfo)
  {
    PlacePageEditCell * cell = [tableView dequeueReusableCellWithIdentifier:[PlacePageEditCell className]];
    cell.titleLabel.text = [self info];
    return cell;
  }
  else if (row == CellRowShare)
  {
    PlacePageShareCell * cell = [tableView dequeueReusableCellWithIdentifier:[PlacePageShareCell className]];
    cell.delegate = self;
    if ([self isMarkOfType:UserMark::API])
      [cell setApiAppTitle:[NSString stringWithUTF8String:GetFramework().GetApiDataHolder().GetAppTitle().c_str()]];
    else
      [cell setApiAppTitle:nil];
    return cell;
  }
  return nil;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  CellRow row = [self cellRowForIndexPath:indexPath];
  if (row == CellRowCommon)
    return [PlacePageInfoCell cellHeightWithAddress:[self address] viewWidth:tableView.width];
  else if (row == CellRowSet)
    return [PlacePageEditCell cellHeightWithTextValue:[self setName] viewWidth:tableView.width];
  else if (row == CellRowInfo)
    return [PlacePageEditCell cellHeightWithTextValue:[self info] viewWidth:tableView.width];
  else if (row == CellRowShare)
    return [PlacePageShareCell cellHeight];
  return 0;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  CellRow row = [self cellRowForIndexPath:indexPath];
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  if (row == CellRowSet)
    [self.delegate placePageView:self willEditProperty:@"Set" inBookmarkAndCategory:GetFramework().FindBookmark(m_mark)];
  else if (row == CellRowInfo)
    [self.delegate placePageView:self willEditProperty:@"Description" inBookmarkAndCategory:GetFramework().FindBookmark(m_mark)];
}

- (void)reloadHeader
{
  self.titleLabel.text = [self title];
  self.titleLabel.width = [self titleWidth];
  [self.titleLabel sizeToFit];
  self.titleLabel.origin = CGPointMake(23, 29);

  self.typeLabel.text = [self types];
  self.typeLabel.width = [self typesWidth];
  [self.typeLabel sizeToFit];
  self.typeLabel.origin = CGPointMake(self.titleLabel.minX + 1, self.titleLabel.maxY + 1);

  self.bookmarkButton.center = CGPointMake(self.headerView.width - 32, 42);
}

- (CGFloat)titleWidth
{
  return self.width - 94;
}

- (CGFloat)typesWidth
{
  return self.width - 94;
}

- (CGFloat)headerHeight
{
  CGFloat titleHeight = [[self title] sizeWithDrawSize:CGSizeMake([self titleWidth], 100) font:self.titleLabel.font].height;
  CGFloat typesHeight = [[self types] sizeWithDrawSize:CGSizeMake([self typesWidth], 30) font:self.typeLabel.font].height;
  return MAX(74, titleHeight + typesHeight + 50);
}

- (void)setState:(PlacePageState)state animated:(BOOL)animated withCallback:(BOOL)withCallback
{
  if (withCallback)
    [self willChangeValueForKey:@"state"];

  [self applyState:state animated:animated];

  if (withCallback)
    [self didChangeValueForKey:@"state"];
}

- (void)applyState:(PlacePageState)state animated:(BOOL)animated
{
  _state = state;
  [self updateBookmarkStateAnimated:NO];
  [self.tableView reloadData];
  [self reloadHeader];
  [self alignAnimated:animated];
  self.tableView.contentInset = UIEdgeInsetsMake([self headerHeight], 0, 0, 0);
  [self.tableView setContentOffset:CGPointMake(0, -self.tableView.contentInset.top) animated:animated];
}

- (void)layoutSubviews
{
  [self alignAnimated:YES];
//  self.tableView.frame = CGRectMake(0, 0, self.superview.width, self.backgroundView.height);
}

- (void)updateHeight:(CGFloat)height
{
  self.height = height;
  self.backgroundView.height = height;
  self.headerView.height = [self headerHeight];

  CALayer * layer = [self.backgroundView.layer.sublayers firstObject];
  layer.frame = self.backgroundView.bounds;

  self.maskLayer1.frame = CGRectMake(0, 0, self.superview.frame.size.width, [self headerHeight]);
  self.maskLayer2.frame = CGRectMake(0, [self headerHeight], self.superview.frame.size.width, self.maskLayer2.superlayer.frame.size.height - [self headerHeight]);
}

- (void)alignAnimated:(BOOL)animated
{
  UIViewAnimationOptions options = UIViewAnimationOptionCurveEaseInOut | UIViewAnimationOptionAllowUserInteraction;
  double damping = 0.95;

  if (self.state == PlacePageStatePreview)
  {
    self.titleLabel.userInteractionEnabled = NO;
    self.arrowImageView.center = CGPointMake(self.width / 2, [self headerHeight] - 10);
    [UIView animateWithDuration:(animated ? 0.4 : 0) delay:0 damping:damping initialVelocity:0 options:options animations:^{
      self.arrowImageView.alpha = 1;
      [self updateHeight:[self headerHeight]];
      self.minY = self.statusBarIncluded ? 0 : -20;
      self.headerSeparator.alpha = 0;
    } completion:^(BOOL finished) {
//      self.tableView.frame = CGRectMake(0, 0, self.superview.width, self.backgroundView.height);
    }];
  }
  else if (self.state == PlacePageStateOpened)
  {
    self.titleLabel.userInteractionEnabled = YES;
    CGFloat fullHeight = [self headerHeight] + [PlacePageInfoCell cellHeightWithAddress:self.address viewWidth:self.tableView.width] + [PlacePageShareCell cellHeight];
    if (self.isBookmark)
    {
      fullHeight += [PlacePageEditCell cellHeightWithTextValue:[self setName] viewWidth:self.tableView.width];
      fullHeight += [PlacePageEditCell cellHeightWithTextValue:[self info] viewWidth:self.tableView.width];
    }
    fullHeight = MIN(fullHeight, [self maxHeight]);
    self.headerSeparator.maxY = [self headerHeight];
    [UIView animateWithDuration:(animated ? 0.4 : 0) delay:0 damping:damping initialVelocity:0 options:options animations:^{
      self.arrowImageView.alpha = 0;
      [self updateHeight:fullHeight];
      self.minY = self.statusBarIncluded ? 0 : -20;
      self.headerSeparator.alpha = 1;
    } completion:^(BOOL finished){
    }];
  }
  else if (self.state == PlacePageStateHidden)
  {
    [UIView animateWithDuration:(animated ? 0.4 : 0) delay:0 damping:damping initialVelocity:0 options:options animations:^{
      self.maxY = 0;
    } completion:nil];
  }

  [self updateEditImageViewAnimated:animated];
}

- (void)updateEditImageViewAnimated:(BOOL)animated
{
  self.editImageView.center = CGPointMake(self.titleLabel.maxX + 14, self.titleLabel.minY + 10.5);
  [UIView animateWithDuration:(animated ? 0.3 : 0) delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    self.editImageView.alpha = self.isBookmark ? (self.state == PlacePageStateOpened ? 1 : 0) : 0;
    PlacePageInfoCell * cell = (PlacePageInfoCell *)[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:ROW_COMMON inSection:0]];
    cell.separator.alpha = self.isBookmark ? 1 : 0;
  } completion:^(BOOL finished) {}];
}

- (void)updateBookmarkStateAnimated:(BOOL)animated
{
  NSIndexPath * indexPath1 = [NSIndexPath indexPathForRow:ROW_SET inSection:0];
  NSIndexPath * indexPath2 = [NSIndexPath indexPathForRow:ROW_INFO inSection:0];
  if (self.bookmarkButton.selected != [self isBookmark])
  {
    NSTimeInterval delay = 0;
    if ([self isBookmark] && !self.bookmarkButton.selected)
    {
      delay = 0;
      [self.tableView insertRowsAtIndexPaths:@[indexPath1, indexPath2] withRowAnimation:UITableViewRowAnimationFade];
    }
    else if (![self isBookmark] && self.bookmarkButton.selected)
    {
      delay = 0.07;
      [self.tableView deleteRowsAtIndexPaths:@[indexPath1, indexPath2] withRowAnimation:UITableViewRowAnimationFade];
    }
    [self performAfterDelay:delay block:^{
      [self willChangeValueForKey:@"state"];
      [self alignAnimated:YES];
      [self didChangeValueForKey:@"state"];
    }];
  }
  self.bookmarkButton.selected = [self isBookmark];
  [self updateEditImageViewAnimated:animated];
}

- (CGFloat)maxHeight
{
  return IPAD ? 600 : (self.superview.width > self.superview.height ? 250 : 430);
}

- (void)didMoveToSuperview
{
  [self setState:PlacePageStateHidden animated:NO withCallback:NO];
}

- (void)metricsChangedNotification:(NSNotification *)notification
{
  [self.tableView reloadRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:ROW_COMMON inSection:0]] withRowAnimation:UITableViewRowAnimationNone];
}

- (void)bookmarkCategoryDeletedNotification:(NSNotification *)notification
{
  if (GetFramework().FindBookmark(m_mark).first == [[notification object] integerValue])
    [self deleteBookmark];
}

- (void)bookmarkDeletedNotification:(NSNotification *)notification
{
  BookmarkAndCategory bookmarkAndCategory;
  [[notification object] getValue:&bookmarkAndCategory];
  if (bookmarkAndCategory == GetFramework().FindBookmark(m_mark))
    [self deleteBookmark];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {}

- (void)swipe:(UISwipeGestureRecognizer *)sender
{
  if (sender.direction == UISwipeGestureRecognizerDirectionUp)
  {
    if (self.state == PlacePageStatePreview)
      [self setState:PlacePageStateHidden animated:YES withCallback:YES];
  }
  else
  {
    if (self.state == PlacePageStatePreview)
      [self setState:PlacePageStateOpened animated:YES withCallback:YES];
  }
}

- (void)titlePress:(UILongPressGestureRecognizer *)sender
{
  UIMenuController * menuController = [UIMenuController sharedMenuController];
  if (!menuController.isMenuVisible)
  {
    CGPoint tapPoint = [sender locationInView:sender.view.superview];
    [menuController setTargetRect:CGRectMake(tapPoint.x, sender.view.maxY - 4, 0, 0) inView:sender.view.superview];
    [menuController setMenuVisible:YES animated:YES];
    [sender.view becomeFirstResponder];
    [menuController update];
  }
}

- (void)infoCellDidPressAddress:(PlacePageInfoCell *)cell withGestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
{
  UIMenuController * menuController = [UIMenuController sharedMenuController];
  if (!menuController.isMenuVisible)
  {
    CGPoint tapPoint = [gestureRecognizer locationInView:gestureRecognizer.view];
    [menuController setTargetRect:CGRectMake(tapPoint.x, 6, 0, 0) inView:gestureRecognizer.view];
    [menuController setMenuVisible:YES animated:YES];
    [gestureRecognizer.view becomeFirstResponder];
    [menuController update];
  }
}

- (void)infoCellDidPressCoordinates:(PlacePageInfoCell *)cell withGestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
{
  UIMenuController * menuController = [UIMenuController sharedMenuController];
  if (!menuController.isMenuVisible)
  {
    CGPoint tapPoint = [gestureRecognizer locationInView:gestureRecognizer.view];
    [menuController setTargetRect:CGRectMake(tapPoint.x, 6, 0, 0) inView:gestureRecognizer.view];
    [menuController setMenuVisible:YES animated:YES];
    [gestureRecognizer.view becomeFirstResponder];
    [menuController update];
  }
}

- (void)titleTap:(UITapGestureRecognizer *)sender
{
  if (self.isBookmark && self.state == PlacePageStateOpened)
    [self.delegate placePageView:self willEditProperty:@"Name" inBookmarkAndCategory:GetFramework().FindBookmark(m_mark)];
}

- (void)tap:(UITapGestureRecognizer *)sender
{
  if (self.state == PlacePageStateOpened)
    [self setState:PlacePageStatePreview animated:YES withCallback:YES];
  else
    [self setState:PlacePageStateOpened animated:YES withCallback:YES];
}

- (void)bookmarkButtonPressed:(UIButton *)sender
{
  if (self.isBookmark)
    [self deleteBookmark];
  else
    [self addBookmark];
}

- (void)guideButtonPressed:(id)sender
{
  NSString * urlString = [NSString stringWithUTF8String:GetFramework().GetApiDataHolder().GetGlobalBackUrl().c_str()];
  [[UIApplication sharedApplication] openURL:[NSURL URLWithString:urlString]];
}

- (void)showUserMark:(UserMark const *)mark
{
  _title = nil;
  _types = nil;
  _address = nil;
  _setName = nil;
  _info = nil;

  m_mark = mark;
  m_cached_mark = mark;

  if ([self isBookmark])
    [self bookmarkActivated:static_cast<Bookmark const *>(mark)];
  else if ([self isMarkOfType:UserMark::API] || [self isMarkOfType:UserMark::POI] || [self isMarkOfType:UserMark::SEARCH])
    [self userMarkActivated:mark];
  GetFramework().ActivateUserMark(mark);
}

- (void)bookmarkActivated:(Bookmark const *)bookmark
{
  m_categoryIndex = GetFramework().FindBookmark(bookmark).first;
  delete m_bookmarkData;
  m_bookmarkData = new BookmarkData(bookmark->GetName(), bookmark->GetType(), bookmark->GetDescription(), bookmark->GetScale(), bookmark->GetTimeStamp());
  m_cached_mark = NULL;
}

- (void)userMarkActivated:(UserMark const *)mark
{
  delete m_bookmarkData;
  m_bookmarkData = NULL;
}

- (BOOL)isMarkOfType:(UserMark::Type)type
{
  if (m_mark == NULL)
    return NO;
  return m_mark->GetMarkType() == type;
}

- (BOOL)isBookmark
{
  return [self isMarkOfType:UserMark::BOOKMARK];
}

- (m2::PointD)pinPoint
{
  return m_mark != NULL ? m_mark->GetOrg() : m2::PointD();
}

- (NSString *)title
{
  if (!_title)
  {
    if ([self isBookmark])
    {
      Bookmark const * bookmark = static_cast<Bookmark const *>(m_mark);
      _title = bookmark->GetName().empty() ? NSLocalizedString(@"dropped_pin", nil) : [NSString stringWithUTF8String:bookmark->GetName().c_str()];
    }
    else if ([self isMarkOfType:UserMark::API])
    {
      ApiMarkPoint const * apiMark = static_cast<ApiMarkPoint const *>(m_mark);
      _title = apiMark->GetName().empty() ? NSLocalizedString(@"dropped_pin", nil) : [NSString stringWithUTF8String:apiMark->GetName().c_str()];
    }
    else if ([self isMarkOfType:UserMark::POI] || [self isMarkOfType:UserMark::SEARCH])
    {
      SearchMarkPoint const * mark = static_cast<SearchMarkPoint const *>(m_mark);
      search::AddressInfo const & addressInfo = mark->GetInfo();
      _title = addressInfo.GetPinName().empty() ? NSLocalizedString(@"dropped_pin", nil) : [NSString stringWithUTF8String:addressInfo.GetPinName().c_str()];
    }
    else if (m_mark != NULL)
    {
      search::AddressInfo addressInfo;
      GetFramework().GetAddressInfoForGlobalPoint(m_mark->GetOrg(), addressInfo);
      _title = addressInfo.GetPinName().empty() ? NSLocalizedString(@"dropped_pin", nil) : [NSString stringWithUTF8String:addressInfo.GetPinName().c_str()];
    }
    else
    {
      _title = @"";
    }
  }
  return _title;
}

- (NSString *)types
{
  if (!_types)
  {
    if ([self isBookmark])
    {
      search::AddressInfo addressInfo;
      GetFramework().GetAddressInfoForGlobalPoint(m_mark->GetOrg(), addressInfo);
      _types = addressInfo.GetPinType().empty() ? @"" : [NSString stringWithUTF8String:addressInfo.GetPinType().c_str()];
    }
    else if ([self isMarkOfType:UserMark::POI] || [self isMarkOfType:UserMark::SEARCH])
    {
      SearchMarkPoint const * mark = static_cast<SearchMarkPoint const *>(m_mark);
      search::AddressInfo const & addressInfo = mark->GetInfo();
      _types = addressInfo.GetPinType().empty() ? @"" : [NSString stringWithUTF8String:addressInfo.GetPinType().c_str()];
    }
    else if (m_mark != NULL)
    {
      search::AddressInfo addressInfo;
      GetFramework().GetAddressInfoForGlobalPoint(m_mark->GetOrg(), addressInfo);
      _types = addressInfo.GetPinType().empty() ? @"" : [NSString stringWithUTF8String:addressInfo.GetPinType().c_str()];
    }
    else
    {
      _types = @"";
    }
  }
  return _types;
}

- (NSString *)setName
{
  if (!_setName)
  {
    if ([self isBookmark])
    {
      Framework & framework = GetFramework();
      BookmarkCategory const * category = framework.GetBmCategory(framework.FindBookmark(m_mark).first);
      _setName = [NSString stringWithUTF8String:category->GetName().c_str()];
    }
    else
    {
      _setName = @"";
    }
  }
  return _setName;
}

- (NSString *)info
{
  if (!_info)
  {
    if ([self isBookmark])
    {
      Bookmark const * bookmark = static_cast<Bookmark const *>(m_mark);
      _info = bookmark->GetDescription().empty() ? NSLocalizedString(@"description", nil) : [NSString stringWithUTF8String:bookmark->GetDescription().c_str()];
    }
    else
    {
      _info = @"";
    }
  }
  return _info;
}

- (NSString *)address
{
  if (!_address)
  {
    if (m_mark != NULL)
    {
      search::AddressInfo addressInfo;
      GetFramework().GetAddressInfoForGlobalPoint(m_mark->GetOrg(), addressInfo);
      _address = [NSString stringWithUTF8String:addressInfo.FormatAddress().c_str()];
    }
    else
    {
      _address = @"";
    }
  }
  return _address;
}

- (UIImage *)iconImageWithImage:(UIImage *)image
{
  CGRect rect = CGRectMake(0, 0, image.size.width, image.size.height);
  UIGraphicsBeginImageContextWithOptions(rect.size, NO, [UIScreen mainScreen].scale);
  [[UIBezierPath bezierPathWithRoundedRect:rect cornerRadius:5.5] addClip];
  [image drawInRect:rect];
  UIImage * iconImage = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();

  return iconImage;
}

- (void)deleteBookmark
{
  Framework & framework = GetFramework();
  BookmarkAndCategory const & bookmarkAndCategory = framework.FindBookmark(m_mark);
  BookmarkCategory * category = framework.GetBmCategory(bookmarkAndCategory.first);
  m_mark = m_cached_mark != NULL ? m_cached_mark : framework.GetAddressMark([self pinPoint]);
  framework.ActivateUserMark(m_mark);
  if (category)
  {
    category->DeleteBookmark(bookmarkAndCategory.second);
    category->SaveToKMLFile();
  }
  framework.Invalidate();

  [self reloadHeader];
  [self updateBookmarkStateAnimated:YES];
}

- (void)addBookmark
{
  Framework & framework = GetFramework();
  if (m_bookmarkData)
  {
    BookmarkCategory const * category = framework.GetBmCategory(m_categoryIndex);
    if (!category)
      m_categoryIndex = framework.LastEditedBMCategory();

    size_t const bookmarkIndex = framework.GetBookmarkManager().AddBookmark(m_categoryIndex, [self pinPoint], *m_bookmarkData);
    m_mark = category->GetBookmark(bookmarkIndex);
  }
  else
  {
    size_t const categoryIndex = framework.LastEditedBMCategory();
    BookmarkData data = BookmarkData([[self title] UTF8String], "placemark-red");
    size_t const bookmarkIndex = framework.AddBookmark(categoryIndex, [self pinPoint], data);
    BookmarkCategory const * category = framework.GetBmCategory(categoryIndex);
    m_mark = category->GetBookmark(bookmarkIndex);
  }
  framework.ActivateUserMark(m_mark);
  framework.Invalidate();

  [self reloadHeader];
  [self updateBookmarkStateAnimated:YES];
}

- (void)shareCellDidPressApiButton:(PlacePageShareCell *)cell
{
  [self.delegate placePageView:self willShareApiPoint:static_cast<ApiMarkPoint const *>(m_mark)];
}

- (void)shareCellDidPressShareButton:(PlacePageShareCell *)cell
{
  [self.delegate placePageView:self willShareText:[self title] point:self.pinPoint];
}

- (UIView *)wrapView
{
  if (!_wrapView)
  {
    _wrapView = [[UIView alloc] initWithFrame:[UIScreen mainScreen].bounds];
    _wrapView.autoresizingMask = UIViewAutoresizingFlexibleWidth;

    CALayer * mask = [CALayer layer];
    mask.frame = _wrapView.bounds;

    CGFloat height1 = [self headerHeight];
    CGFloat height2 = _wrapView.height - height1;

    CALayer * submask1 = [CALayer layer];
    submask1.frame = CGRectMake(0, 0, _wrapView.width, height1);
    submask1.backgroundColor = [[UIColor clearColor] CGColor];
    [mask addSublayer:submask1];
    self.maskLayer1 = submask1;

    CALayer * submask2 = [CALayer layer];
    submask2.frame = CGRectMake(0, height1, _wrapView.width, height2);
    submask2.backgroundColor = [[UIColor blackColor] CGColor];
    [mask addSublayer:submask2];
    self.maskLayer2 = submask2;

    [_wrapView.layer addSublayer:mask];
    _wrapView.layer.mask = mask;
  }
  return _wrapView;
}

- (CopyLabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[CopyLabel alloc] initWithFrame:CGRectZero];
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:17.5];
    _titleLabel.textColor = [UIColor whiteColor];
    _titleLabel.numberOfLines = 0;
    _titleLabel.lineBreakMode = NSLineBreakByWordWrapping;
    UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(titleTap:)];
    [_titleLabel addGestureRecognizer:tap];
    UILongPressGestureRecognizer * press = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(titlePress:)];
    [_titleLabel addGestureRecognizer:press];
  }
  return _titleLabel;
}

- (UILabel *)typeLabel
{
  if (!_typeLabel)
  {
    _typeLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _typeLabel.backgroundColor = [UIColor clearColor];
    _typeLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:15];
    _typeLabel.textAlignment = NSTextAlignmentLeft;
    _typeLabel.textColor = [UIColor whiteColor];
    _typeLabel.numberOfLines = 0;
    _typeLabel.lineBreakMode = NSLineBreakByWordWrapping;
  }
  return _typeLabel;
}

- (UIButton *)bookmarkButton
{
  if (!_bookmarkButton)
  {
    _bookmarkButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 64, 44)];
    _bookmarkButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
    [_bookmarkButton setImage:[UIImage imageNamed:@"PlacePageBookmarkButton"] forState:UIControlStateNormal];
    [_bookmarkButton setImage:[UIImage imageNamed:@"PlacePageBookmarkButtonSelected"] forState:UIControlStateSelected];
    [_bookmarkButton addTarget:self action:@selector(bookmarkButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _bookmarkButton;
}

- (UIView *)headerView
{
  if (!_headerView)
  {
    _headerView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, self.tableView.width, [self headerHeight])];
    _headerView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleBottomMargin;

    [_headerView addSubview:self.titleLabel];
    [_headerView addSubview:self.typeLabel];
    [_headerView addSubview:self.bookmarkButton];
    self.bookmarkButton.center = CGPointMake(_headerView.width - 32, 42);

    UIImage * separatorImage = [[UIImage imageNamed:@"PlacePageSeparator"] resizableImageWithCapInsets:UIEdgeInsetsZero];
    CGFloat const offset = 15;
    UIImageView * separator = [[UIImageView alloc] initWithFrame:CGRectMake(offset, _headerView.height - separatorImage.size.height, _headerView.width - 2 * offset, separatorImage.size.height)];
    separator.image = separatorImage;
    separator.maxY = _headerView.height;
    separator.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    [_headerView addSubview:separator];
    self.headerSeparator = separator;

    [_headerView addSubview:self.editImageView];

    UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap:)];
    [_headerView addGestureRecognizer:tap];
  }
  return _headerView;
}

- (UIImageView *)editImageView
{
  if (!_editImageView)
  {
    _editImageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"PlacePageEditButton"]];
    _editImageView.userInteractionEnabled = YES;
    UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(titleTap:)];
    [_editImageView addGestureRecognizer:tap];
  }
  return _editImageView;
}

- (UITableView *)tableView
{
  if (!_tableView)
  {
    _tableView = [[UITableView alloc] initWithFrame:[UIScreen mainScreen].bounds];
    _tableView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    _tableView.delegate = self;
    _tableView.dataSource = self;
    _tableView.backgroundColor = [UIColor clearColor];
    _tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
    _tableView.alwaysBounceVertical = NO;
  }
  return _tableView;
}

- (UIView *)backgroundView
{
  if (!_backgroundView)
  {
    _backgroundView = [[UIView alloc] initWithFrame:self.bounds];
    _backgroundView.autoresizingMask = UIViewAutoresizingFlexibleWidth;

    CAGradientLayer * gradient = [CAGradientLayer layer];
    gradient.colors = @[(id)[[UIColor colorWithColorCode:@"15d180"] CGColor], (id)[[UIColor colorWithColorCode:@"16b68a"] CGColor]];
    gradient.startPoint = CGPointMake(0, 0);
    gradient.endPoint = CGPointMake(0, 1);
    gradient.locations = @[@0, @1];
    gradient.frame = _backgroundView.bounds;

    [_backgroundView.layer addSublayer:gradient];
  }
  return _backgroundView;
}

- (UIImageView *)arrowImageView
{
  if (!_arrowImageView)
  {
    _arrowImageView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    _arrowImageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"PlacePageArrow"]];
  }
  return _arrowImageView;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end
