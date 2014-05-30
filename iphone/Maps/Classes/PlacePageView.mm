
#import "PlacePageView.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "BookmarksRootVC.h"
#import "Framework.h"
#import "ContextViews.h"
#import "PlacePageInfoCell.h"
#import "PlacePageEditCell.h"
#import "PlacePageShareCell.h"
#include "../../search/result.hpp"
#import "ColorPickerView.h"
#import "Statistics.h"
#include "../../std/shared_ptr.hpp"

typedef NS_ENUM(NSUInteger, CellRow)
{
  CellRowCommon,
  CellRowSet,
  CellRowInfo,
  CellRowShare,
};

@interface PlacePageView () <UIGestureRecognizerDelegate, UITableViewDataSource, UITableViewDelegate, LocationObserver, PlacePageShareCellDelegate, PlacePageInfoCellDelegate, ColorPickerDelegate, UIAlertViewDelegate>

@property (nonatomic) UIView * backgroundView;
@property (nonatomic) UIView * headerView;
@property (nonatomic) UIView * headerSeparator;
@property (nonatomic) UITableView * tableView;
@property (nonatomic) CopyLabel * titleLabel;
@property (nonatomic) UIButton * bookmarkButton;
@property (nonatomic) UILabel * typeLabel;
@property (nonatomic) UIButton * shareButton;
@property (nonatomic) UIImageView * editImageView;
@property (nonatomic) UIImageView * arrowImageView;
@property (nonatomic) UIView * pickerView;

@property (nonatomic) UIView * swipeView;

@property (nonatomic) NSString * title;
@property (nonatomic) NSString * types;
@property (nonatomic) NSString * address;
@property (nonatomic) NSString * info;
@property (nonatomic) NSString * setName;

- (NSString *)colorName;

@end

@implementation PlacePageView
{
  shared_ptr<UserMarkCopy> m_mark;
  shared_ptr<UserMarkCopy> m_cachedMark;
  BookmarkData * m_bookmarkData;
  size_t m_categoryIndex;
  BOOL updatingTable;
}

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  self.clipsToBounds = YES;

  m_mark.reset();
  m_cachedMark.reset();
  m_bookmarkData = NULL;

  UISwipeGestureRecognizer * swipeUp = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipe:)];
  swipeUp.direction = UISwipeGestureRecognizerDirectionUp;
  [self addGestureRecognizer:swipeUp];

  UISwipeGestureRecognizer * swipeDown = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipe:)];
  swipeDown.direction = UISwipeGestureRecognizerDirectionDown;
  [self addGestureRecognizer:swipeDown];

  [self addSubview:self.backgroundView];
  [self addSubview:self.tableView];
  [self addSubview:self.headerView];
  [self addSubview:self.arrowImageView];
  [self addSubview:self.swipeView];

  NSNotificationCenter * nc = [NSNotificationCenter defaultCenter];
  [nc addObserver:self selector:@selector(bookmarkDeletedNotification:) name:BOOKMARK_DELETED_NOTIFICATION object:nil];
  [nc addObserver:self selector:@selector(bookmarkCategoryDeletedNotification:) name:BOOKMARK_CATEGORY_DELETED_NOTIFICATION object:nil];
  [nc addObserver:self selector:@selector(metricsChangedNotification:) name:METRICS_CHANGED_NOTIFICATION object:nil];

  [self.tableView registerClass:[PlacePageInfoCell class] forCellReuseIdentifier:[PlacePageInfoCell className]];
  [self.tableView registerClass:[PlacePageEditCell class] forCellReuseIdentifier:[PlacePageEditCell className]];
  [self.tableView registerClass:[PlacePageShareCell class] forCellReuseIdentifier:[PlacePageShareCell className]];

  CGFloat const defaultHeight = 93;
  [self updateHeight:defaultHeight];

  updatingTable = NO;

  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(startMonitoringLocation:) name:LOCATION_MANAGER_STARTED_NOTIFICATION object:nil];

  return self;
}

- (void)startMonitoringLocation:(NSNotification *)notification
{
  [[MapsAppDelegate theApp].m_locationManager start:self];
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
    [cell setAddress:self.address pinPoint:[self pinPoint]];
    [cell setColor:[ColorPickerView colorForName:[self colorName]]];
    cell.selectedColorView.alpha = [self isBookmark] ? 1 : 0;
    cell.delegate = self;
    return cell;
  }
  else if (row == CellRowSet)
  {
    PlacePageEditCell * cell = [tableView dequeueReusableCellWithIdentifier:[PlacePageEditCell className]];
    cell.titleLabel.text = self.setName;
    return cell;
  }
  else if (row == CellRowInfo)
  {
    PlacePageEditCell * cell = [tableView dequeueReusableCellWithIdentifier:[PlacePageEditCell className]];
    cell.titleLabel.text = self.info;
    return cell;
  }
  else if (row == CellRowShare)
  {
    PlacePageShareCell * cell = [tableView dequeueReusableCellWithIdentifier:[PlacePageShareCell className]];
    cell.delegate = self;
    if ([self isMarkOfType:UserMark::API])
      cell.apiAppTitle = [NSString stringWithUTF8String:GetFramework().GetApiDataHolder().GetAppTitle().c_str()];
    else
      cell.apiAppTitle = nil;
    return cell;
  }
  return nil;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  CellRow row = [self cellRowForIndexPath:indexPath];
  if (row == CellRowCommon)
    return [PlacePageInfoCell cellHeightWithAddress:self.address viewWidth:tableView.width];
  else if (row == CellRowSet)
    return [PlacePageEditCell cellHeightWithTextValue:self.setName viewWidth:tableView.width];
  else if (row == CellRowInfo)
    return [PlacePageEditCell cellHeightWithTextValue:self.info viewWidth:tableView.width];
  else if (row == CellRowShare)
    return [PlacePageShareCell cellHeight];
  return 0;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  CellRow row = [self cellRowForIndexPath:indexPath];
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  if (row == CellRowSet)
    [self.delegate placePageView:self willEditProperty:@"Set" inBookmarkAndCategory:GetFramework().FindBookmark([self userMark])];
  else if (row == CellRowInfo)
    [self.delegate placePageView:self willEditProperty:@"Description" inBookmarkAndCategory:GetFramework().FindBookmark([self userMark])];
}

- (void)reloadHeader
{
  self.titleLabel.text = self.title;
  self.titleLabel.width = [self titleWidth];
  [self.titleLabel sizeToFit];
  self.titleLabel.origin = CGPointMake(23, 29);

  self.typeLabel.text = self.types;
  self.typeLabel.width = [self typesWidth];
  [self.typeLabel sizeToFit];
  self.typeLabel.origin = CGPointMake(self.titleLabel.minX + 1, self.titleLabel.maxY + 1);

  self.bookmarkButton.center = CGPointMake(self.headerView.width - 32, 42);
}

- (CGFloat)titleWidth
{
  return self.width - 90;
}

- (CGFloat)typesWidth
{
  return self.width - 90;
}

- (CGFloat)headerHeight
{
  CGFloat titleHeight = [self.title sizeWithDrawSize:CGSizeMake([self titleWidth], 100) font:self.titleLabel.font].height;
  CGFloat typesHeight = [self.types sizeWithDrawSize:CGSizeMake([self typesWidth], 30) font:self.typeLabel.font].height;
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
  [self updateBookmarkViewsAlpha:animated];
  [self.tableView reloadData];
  [self reloadHeader];
  [self alignAnimated:animated];
  self.tableView.contentInset = UIEdgeInsetsMake([self headerHeight], 0, 0, 0);
  [self.tableView setContentOffset:CGPointMake(0, -self.tableView.contentInset.top) animated:animated];
}

- (void)layoutSubviews
{
  if (!updatingTable)
  {
    [self setState:self.state animated:YES withCallback:YES];
    self.tableView.frame = CGRectMake(0, 0, self.superview.width, self.backgroundView.height);
//    [self reloadHeader];
//    [self alignAnimated:YES];
//    self.tableView.contentInset = UIEdgeInsetsMake([self headerHeight], 0, 0, 0);
    self.swipeView.frame = self.bounds;
  }
}

- (void)updateHeight:(CGFloat)height
{
  self.height = height;
  self.backgroundView.height = height;
  self.headerView.height = [self headerHeight];

  CALayer * layer = [self.backgroundView.layer.sublayers firstObject];
  layer.frame = self.backgroundView.bounds;
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
      self.minY = self.statusBarIncluded ? (SYSTEM_VERSION_IS_LESS_THAN(@"7") ? -20 : 0) : -20;
      self.headerSeparator.alpha = 0;
    } completion:nil];
  }
  else if (self.state == PlacePageStateOpened)
  {
    self.titleLabel.userInteractionEnabled = YES;
    CGFloat fullHeight = [self headerHeight] + [PlacePageInfoCell cellHeightWithAddress:self.address viewWidth:self.tableView.width] + [PlacePageShareCell cellHeight];
    if (self.isBookmark)
    {
      fullHeight += [PlacePageEditCell cellHeightWithTextValue:self.setName viewWidth:self.tableView.width];
      fullHeight += [PlacePageEditCell cellHeightWithTextValue:self.info viewWidth:self.tableView.width];
    }
    fullHeight = MIN(fullHeight, [self maxHeight]);
    self.headerSeparator.maxY = [self headerHeight];
    [UIView animateWithDuration:(animated ? 0.4 : 0) delay:0 damping:damping initialVelocity:0 options:options animations:^{
      self.arrowImageView.alpha = 0;
      [self updateHeight:fullHeight];
      self.minY = self.statusBarIncluded ? (SYSTEM_VERSION_IS_LESS_THAN(@"7") ? -20 : 0) : -20;
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

  [self updateBookmarkViewsAlpha:animated];
}

- (void)updateBookmarkViewsAlpha:(BOOL)animated
{
  self.editImageView.center = CGPointMake(self.titleLabel.maxX + 14, self.titleLabel.minY + 10.5);
  [UIView animateWithDuration:(animated ? 0.3 : 0) delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    self.editImageView.alpha = [self isBookmark] ? (self.state == PlacePageStateOpened ? 1 : 0) : 0;
    PlacePageInfoCell * cell = (PlacePageInfoCell *)[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:ROW_COMMON inSection:0]];
    cell.separator.alpha = [self isBookmark] ? 1 : 0;
    cell.selectedColorView.alpha = [self isBookmark] ? 1 : 0;
  } completion:nil];
}

- (void)updateBookmarkStateAnimated:(BOOL)animated
{
  if ([self isBookmark])
  {
    CGFloat newHeight = self.backgroundView.height + [PlacePageEditCell cellHeightWithTextValue:self.info viewWidth:self.tableView.width] + [PlacePageEditCell cellHeightWithTextValue:self.setName viewWidth:self.tableView.width];
    self.tableView.frame = CGRectMake(0, 0, self.superview.width, newHeight);
  }

//  [self performAfterDelay:0 block:^{
    [self resizeTableAnimated:animated];
//  }];
}

- (void)resizeTableAnimated:(BOOL)animated
{
  if (self.bookmarkButton.selected != [self isBookmark])
  {
    updatingTable = YES;

    NSIndexPath * indexPath1 = [NSIndexPath indexPathForRow:ROW_SET inSection:0];
    NSIndexPath * indexPath2 = [NSIndexPath indexPathForRow:ROW_INFO inSection:0];

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
      [self performAfterDelay:0 block:^{
        updatingTable = NO;
      }];
    }];
    self.bookmarkButton.selected = [self isBookmark];
  }
}

- (CGFloat)maxHeight
{
  return IPAD ? 600 : (self.superview.width > self.superview.height ? 270 : 470);
}

- (void)didMoveToSuperview
{
  [self setState:PlacePageStateHidden animated:NO withCallback:NO];
}

- (void)metricsChangedNotification:(NSNotification *)notification
{
  [self.tableView reloadRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:ROW_COMMON inSection:0]] withRowAnimation:UITableViewRowAnimationNone];
}

- (void)abortBookmarkState
{
  Framework & framework = GetFramework();
  m_mark = [self cachedMark];
  framework.ActivateUserMark([self userMark]);

  [self clearCachedProperties];
  [self reloadHeader];
  [self updateBookmarkStateAnimated:NO];
  [self updateBookmarkViewsAlpha:NO];
}

- (void)bookmarkCategoryDeletedNotification:(NSNotification *)notification
{
  if (m_categoryIndex == [[notification object] integerValue])
    [self abortBookmarkState];
}

- (void)bookmarkDeletedNotification:(NSNotification *)notification
{
  BookmarkAndCategory bookmarkAndCategory;
  [[notification object] getValue:&bookmarkAndCategory];
  if (bookmarkAndCategory == GetFramework().FindBookmark([self userMark]))
    [self abortBookmarkState];
}

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

- (void)infoCellDidPressColorSelector:(PlacePageInfoCell *)cell
{
  UIWindow * appWindow = [[UIApplication sharedApplication].windows firstObject];
  UIWindow * window = [[appWindow subviews] firstObject];

  self.pickerView = [[UIView alloc] initWithFrame:window.bounds];
  ColorPickerView * colorPicker = [[ColorPickerView alloc] initWithWidth:(IPAD ? 340 : 300) andSelectButton:[ColorPickerView getColorIndex:self.colorName]];
  colorPicker.delegate = self;
  colorPicker.minX = (self.pickerView.width - colorPicker.width) / 2;
  colorPicker.minY = (self.pickerView.height - colorPicker.height) / 2;
  [self.pickerView addSubview:colorPicker];
  self.pickerView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  self.pickerView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.5];
  UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(dismissPicker)];
  [self.pickerView addGestureRecognizer:tap];

  [window addSubview:self.pickerView];
}

- (void)dismissPicker
{
  [self.pickerView removeFromSuperview];
}

- (void)colorPicked:(size_t)colorIndex
{
  PlacePageInfoCell * cell = (PlacePageInfoCell *)[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:ROW_COMMON inSection:0]];
  UIColor * color = [ColorPickerView colorForName:[ColorPickerView colorName:colorIndex]];
  [cell setColor:color];

  Framework & framework = GetFramework();
  UserMark const * mark = [self userMark];
  BookmarkAndCategory bookmarkAndCategory = framework.FindBookmark(mark);
  BookmarkCategory * category = GetFramework().GetBmCategory(bookmarkAndCategory.first);
  Bookmark * bookmark = category->GetBookmark(bookmarkAndCategory.second);
  bookmark->SetType([[ColorPickerView colorName:colorIndex] UTF8String]);
  category->SaveToKMLFile();

  framework.ActivateUserMark(mark);
  framework.Invalidate();

  [self dismissPicker];
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
    [self.delegate placePageView:self willEditProperty:@"Name" inBookmarkAndCategory:GetFramework().FindBookmark([self userMark])];
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
  if ([self isBookmark])
    [self deleteBookmark];
  else
    [self addBookmark];
}

- (void)guideButtonPressed:(id)sender
{
  NSString * urlString = [NSString stringWithUTF8String:GetFramework().GetApiDataHolder().GetGlobalBackUrl().c_str()];
  [[UIApplication sharedApplication] openURL:[NSURL URLWithString:urlString]];
}

- (void)clearCachedProperties
{
  _title = nil;
  _types = nil;
  _address = nil;
  _setName = nil;
  _info = nil;
}

- (void)showUserMark:(UserMarkCopy *)mark
{
  [self clearCachedProperties];

  m_mark.reset(mark);
  m_cachedMark = m_mark;

  UserMark const * innerMark = [self userMark];
  if ([self isBookmark])
    [self bookmarkActivated:static_cast<Bookmark const *>(innerMark)];
  else if ([self isMarkOfType:UserMark::API] || [self isMarkOfType:UserMark::POI] || [self isMarkOfType:UserMark::SEARCH])
    [self userMarkActivated:innerMark];
  GetFramework().ActivateUserMark(innerMark);
}

- (void)bookmarkActivated:(Bookmark const *)bookmark
{
  m_categoryIndex = GetFramework().FindBookmark(bookmark).first;
  delete m_bookmarkData;
  m_bookmarkData = new BookmarkData(bookmark->GetName(), bookmark->GetType(), bookmark->GetDescription(), bookmark->GetScale(), bookmark->GetTimeStamp());
  m_cachedMark.reset();
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
  return [self userMark]->GetMarkType() == type;
}

- (BOOL)isBookmark
{
  return [self isMarkOfType:UserMark::BOOKMARK];
}

- (m2::PointD)pinPoint
{
  return m_mark != NULL ? [self userMark]->GetOrg() : m2::PointD();
}

- (NSString *)title
{
  if (!_title)
  {
    if ([self isBookmark])
    {
      Bookmark const * bookmark = static_cast<Bookmark const *>([self userMark]);
      _title = bookmark->GetName().empty() ? NSLocalizedString(@"dropped_pin", nil) : [NSString stringWithUTF8String:bookmark->GetName().c_str()];
    }
    else if ([self isMarkOfType:UserMark::API])
    {
      ApiMarkPoint const * apiMark = static_cast<ApiMarkPoint const *>([self userMark]);
      _title = apiMark->GetName().empty() ? NSLocalizedString(@"dropped_pin", nil) : [NSString stringWithUTF8String:apiMark->GetName().c_str()];
    }
    else if ([self isMarkOfType:UserMark::POI] || [self isMarkOfType:UserMark::SEARCH])
    {
      SearchMarkPoint const * mark = static_cast<SearchMarkPoint const *>([self userMark]);
      search::AddressInfo const & addressInfo = mark->GetInfo();
      _title = addressInfo.GetPinName().empty() ? NSLocalizedString(@"dropped_pin", nil) : [NSString stringWithUTF8String:addressInfo.GetPinName().c_str()];
    }
    else if (m_mark != NULL)
    {
      search::AddressInfo addressInfo;
      GetFramework().GetAddressInfoForGlobalPoint([self userMark]->GetOrg(), addressInfo);
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
      GetFramework().GetAddressInfoForGlobalPoint([self userMark]->GetOrg(), addressInfo);
      _types = addressInfo.GetPinType().empty() ? @"" : [NSString stringWithUTF8String:addressInfo.GetPinType().c_str()];
    }
    else if ([self isMarkOfType:UserMark::POI] || [self isMarkOfType:UserMark::SEARCH])
    {
      SearchMarkPoint const * mark = static_cast<SearchMarkPoint const *>([self userMark]);
      search::AddressInfo const & addressInfo = mark->GetInfo();
      _types = addressInfo.GetPinType().empty() ? @"" : [NSString stringWithUTF8String:addressInfo.GetPinType().c_str()];
    }
    else if (m_mark != NULL)
    {
      search::AddressInfo addressInfo;
      GetFramework().GetAddressInfoForGlobalPoint([self userMark]->GetOrg(), addressInfo);
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
      BookmarkCategory const * category = framework.GetBmCategory(framework.FindBookmark([self userMark]).first);
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
      Bookmark const * bookmark = static_cast<Bookmark const *>([self userMark]);
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
      GetFramework().GetAddressInfoForGlobalPoint([self userMark]->GetOrg(), addressInfo);
      _address = [NSString stringWithUTF8String:addressInfo.FormatAddress().c_str()];
    }
    else
    {
      _address = @"";
    }
  }
  return _address;
}

- (NSString *)colorName
{
  if ([self isBookmark])
  {
    Bookmark const * bookmark = static_cast<Bookmark const *>([self userMark]);
    return bookmark->GetType().empty() ? @"placemark-red" : [NSString stringWithUTF8String:bookmark->GetType().c_str()];
  }
  else
  {
    return @"placemark-red";
  }
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

- (shared_ptr<UserMarkCopy>)cachedMark
{
  return (m_cachedMark != NULL) ? m_cachedMark : make_shared_ptr(GetFramework().GetAddressMark([self pinPoint])->Copy());
}

- (void)deleteBookmark
{
  Framework & framework = GetFramework();
  UserMark const * mark = [self userMark];
  BookmarkAndCategory const & bookmarkAndCategory = framework.FindBookmark(mark);
  BookmarkCategory * category = framework.GetBmCategory(bookmarkAndCategory.first);
  m_mark = [self cachedMark];
  framework.ActivateUserMark(mark);
  if (category)
  {
    category->DeleteBookmark(bookmarkAndCategory.second);
    category->SaveToKMLFile();
  }
  framework.Invalidate();

  [self reloadHeader];
  [self updateBookmarkStateAnimated:YES];
  [self updateBookmarkViewsAlpha:YES];
}

- (void)addBookmark
{
  if (GetPlatform().IsPro())
  {
    Framework & framework = GetFramework();
    if (m_bookmarkData)
    {
      BookmarkCategory const * category = framework.GetBmCategory(m_categoryIndex);
      if (!category)
        m_categoryIndex = framework.LastEditedBMCategory();

      size_t const bookmarkIndex = framework.GetBookmarkManager().AddBookmark(m_categoryIndex, [self pinPoint], *m_bookmarkData);
      m_mark = make_shared_ptr(category->GetBookmark(bookmarkIndex)->Copy());
    }
    else
    {
      size_t const categoryIndex = framework.LastEditedBMCategory();
      BookmarkData data = BookmarkData([self.title UTF8String], "placemark-red");
      size_t const bookmarkIndex = framework.AddBookmark(categoryIndex, [self pinPoint], data);
      BookmarkCategory const * category = framework.GetBmCategory(categoryIndex);
      m_mark = make_shared_ptr(category->GetBookmark(bookmarkIndex)->Copy());
    }
    framework.ActivateUserMark([self userMark]);
    framework.Invalidate();

    [self reloadHeader];
    [self updateBookmarkStateAnimated:YES];
    [self updateBookmarkViewsAlpha:YES];
  }
  else
  {
    UIAlertView * alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"bookmarks_in_pro_version", nil) message:nil delegate:self cancelButtonTitle:NSLocalizedString(@"cancel", nil) otherButtonTitles:NSLocalizedString(@"get_it_now", nil), nil];
    [alert show];
  }
}

- (void)shareCellDidPressApiButton:(PlacePageShareCell *)cell
{
  [self.delegate placePageView:self willShareApiPoint:static_cast<ApiMarkPoint const *>([self userMark])];
}

- (void)shareCellDidPressShareButton:(PlacePageShareCell *)cell
{
  [self.delegate placePageView:self willShareText:self.title point:self.pinPoint];
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
  UIScrollView * sv = scrollView;
  if ((sv.contentOffset.y + sv.height > sv.contentSize.height + 70) && !sv.dragging && sv.decelerating)
  {
    if (self.state == PlacePageStateOpened)
      [self setState:PlacePageStatePreview animated:YES withCallback:YES];
  }
}

- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView withVelocity:(CGPoint)velocity targetContentOffset:(inout CGPoint *)targetContentOffset
{
  NSLog(@"%f %f", scrollView.contentSize.height, scrollView.height);
  if (scrollView.contentSize.height <= scrollView.height && (*targetContentOffset).y <= scrollView.contentOffset.y)
  {
    [self setState:PlacePageStateHidden animated:YES withCallback:YES];
  }
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (buttonIndex != alertView.cancelButtonIndex)
  {
    [[Statistics instance] logProposalReason:@"Add Bookmark" withAnswer:@"YES"];
    [[UIApplication sharedApplication] openProVersionFrom:@"ios_pp_bookmark"];
  }
  else
  {
    [[Statistics instance] logProposalReason:@"Add Bookmark" withAnswer:@"NO"];
  }
}

-(UserMark const *) userMark
{
  if (m_mark)
    return m_mark->GetUserMark();
    
  return NULL;
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
    _headerView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, self.width, [self headerHeight])];
    _headerView.backgroundColor = [UIColor applicationColor];
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
