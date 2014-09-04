
#import "PlacePageView.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "BookmarksRootVC.h"
#import "Framework.h"
#import "ContextViews.h"
#import "PlacePageInfoCell.h"
#import "PlacePageEditCell.h"
#import "PlacePageShareCell.h"
#import "PlacePageRoutingCell.h"
#import "PlacePageBookmarkDescriptionCell.h"
#include "../../search/result.hpp"
#import "ColorPickerView.h"
#import "Statistics.h"
#include "../../std/shared_ptr.hpp"

typedef NS_ENUM(NSUInteger, CellRow)
{
  CellRowCommon,
  CellRowSet,
  CellRowBookmarkDescription,
  CellRowShare,
  CellRowRouting,
  CellInvalid     // Sections changed but rows are not
};

@interface PlacePageView () <UIGestureRecognizerDelegate, UITableViewDataSource, UITableViewDelegate, PlacePageShareCellDelegate, PlacePageInfoCellDelegate, ColorPickerDelegate, UIAlertViewDelegate, PlacePageRoutingCellDelegate, UIWebViewDelegate>

@property (nonatomic) UIImageView * backgroundView;
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
@property (nonatomic) UIWebView * bookmarkDescriptionView;

@property (nonatomic) NSString * title;
@property (nonatomic) NSString * types;
@property (nonatomic) NSString * address;
@property (nonatomic) NSString * bookmarkDescription;
@property (nonatomic) NSString * setName;

@property (nonatomic) NSString * temporaryTitle;

@end

@implementation PlacePageView
{
  shared_ptr<UserMarkCopy> m_mark;
  shared_ptr<UserMarkCopy> m_cachedMark;
  BookmarkData * m_bookmarkData;
  size_t m_categoryIndex;
  BOOL updatingTable;
  BOOL needToUpdateWebView;
  BOOL webViewIsLoaded;

  BOOL needUpdateAddressInfo;
  search::AddressInfo m_addressInfo;
  BOOL m_hasSpeed;
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

  NSNotificationCenter * nc = [NSNotificationCenter defaultCenter];
  [nc addObserver:self selector:@selector(bookmarkDeletedNotification:) name:BOOKMARK_DELETED_NOTIFICATION object:nil];
  [nc addObserver:self selector:@selector(bookmarkCategoryDeletedNotification:) name:BOOKMARK_CATEGORY_DELETED_NOTIFICATION object:nil];
  [nc addObserver:self selector:@selector(metricsChangedNotification:) name:METRICS_CHANGED_NOTIFICATION object:nil];

  if ([self.tableView respondsToSelector:@selector(registerClass:forCellReuseIdentifier:)])
  {
    // only for iOS 6 and higher
    [self.tableView registerClass:[PlacePageInfoCell class] forCellReuseIdentifier:[PlacePageInfoCell className]];
    [self.tableView registerClass:[PlacePageEditCell class] forCellReuseIdentifier:[PlacePageEditCell className]];
    [self.tableView registerClass:[PlacePageShareCell class] forCellReuseIdentifier:[PlacePageShareCell className]];
    [self.tableView registerClass:[PlacePageRoutingCell class] forCellReuseIdentifier:[PlacePageRoutingCell className]];
  }

  CGFloat const defaultHeight = 93;
  [self updateHeight:defaultHeight];

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
  PlacePageInfoCell * cell = (PlacePageInfoCell *)[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:ROW_COMMON inSection:0]];
  [cell updateDistance];

  if ([self isMyPosition])
  {
    BOOL hasSpeed = NO;
    self.typeLabel.text = [[MapsAppDelegate theApp].m_locationManager formattedSpeedAndAltitude:hasSpeed];
    if (hasSpeed != m_hasSpeed)
    {
      m_hasSpeed = hasSpeed;
      [self reloadHeader];
    }
    if (self.state == PlacePageStateOpened)
      [cell updateCoordinates];
  }
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  double lat, lon;
  if (![[MapsAppDelegate theApp].m_locationManager getLat:lat Lon:lon])
    return;
  double const northRad = (info.m_trueHeading < 0) ? info.m_magneticHeading : info.m_trueHeading;
  m2::PointD const point = MercatorBounds::FromLatLon(lat, lon);

  PlacePageInfoCell * cell = (PlacePageInfoCell *)[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:ROW_COMMON inSection:0]];
  cell.compassView.angle = ang::AngleTo(point, [self pinPoint]) + northRad;
}

- (CellRow)cellRowForIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.row == 0)
    return CellRowCommon;
  else if (indexPath.row == 1)
    return [self isBookmark] ? CellRowSet : CellRowShare;
  else if (indexPath.row == 2)
    return [self isBookmark] ? CellRowBookmarkDescription : CellRowRouting;
  else if (indexPath.row == 3)
    return CellRowShare;
  else if (indexPath.row == 4)
    return CellRowRouting;
  return CellInvalid;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self rowsCount];
}

- (NSInteger)rowsCount
{
  Framework & f = GetFramework();
  if ([self isBookmark])
    return f.IsRoutingEnabled() ? 5 : 4;
  else
    return f.IsRoutingEnabled() ? 3 : 2;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  CellRow row = [self cellRowForIndexPath:indexPath];
  if (row == CellRowCommon)
  {
    PlacePageInfoCell * cell = [tableView dequeueReusableCellWithIdentifier:[PlacePageInfoCell className]];
    if (!cell) // only for iOS 5
      cell = [[PlacePageInfoCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[PlacePageInfoCell className]];

    cell.pinPoint = [self pinPoint];
    cell.color = [ColorPickerView colorForName:[self colorName]];
    cell.selectedColorView.alpha = [self isBookmark] ? 1 : 0;
    cell.delegate = self;
    cell.myPositionMode = [self isMyPosition];

    return cell;
  }
  else if (row == CellRowSet)
  {
    PlacePageEditCell * cell = [tableView dequeueReusableCellWithIdentifier:[PlacePageEditCell className]];
    if (!cell) // only for iOS 5
      cell = [[PlacePageEditCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[PlacePageEditCell className]];

    cell.titleLabel.text = self.setName;
    return cell;
  }
  else if (row == CellRowBookmarkDescription)
  {
    PlacePageBookmarkDescriptionCell * cell = [tableView dequeueReusableCellWithIdentifier:[PlacePageBookmarkDescriptionCell className]];
    if (!cell) // only for iOS 5
      cell = [[PlacePageBookmarkDescriptionCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[PlacePageBookmarkDescriptionCell className]];
    if ([self descriptionIsHTML])
    {
      cell.webView.hidden = NO;
      cell.titleLabel.hidden = YES;
      cell.webView = self.bookmarkDescriptionView;
      if (!webViewIsLoaded)
        [self.bookmarkDescriptionView loadHTMLString:self.bookmarkDescription baseURL:nil];
    }
    else
    {
      cell.titleLabel.hidden = NO;
      cell.webView.hidden = YES;
      cell.titleLabel.text = self.bookmarkDescription;
    }
    return cell;
  }
  else if (row == CellRowShare)
  {
    PlacePageShareCell * cell = [tableView dequeueReusableCellWithIdentifier:[PlacePageShareCell className]];
    if (!cell) // only for iOS 5
      cell = [[PlacePageShareCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[PlacePageShareCell className]];

    cell.delegate = self;
    if ([self isMarkOfType:UserMark::API])
      cell.apiAppTitle = [NSString stringWithUTF8String:GetFramework().GetApiDataHolder().GetAppTitle().c_str()];
    else
      cell.apiAppTitle = nil;
    return cell;
  }
  else if (row == CellRowRouting)
  {
    PlacePageRoutingCell * cell = [tableView dequeueReusableCellWithIdentifier:[PlacePageRoutingCell className]];
    if (!cell) // only for iOS 5
      cell = [[PlacePageRoutingCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[PlacePageRoutingCell className]];

    cell.delegate = self;
    return cell;
  }
  return nil;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  CellRow row = [self cellRowForIndexPath:indexPath];
  if (row == CellRowCommon)
    return [PlacePageInfoCell cellHeightWithViewWidth:tableView.width inMyPositionMode:[self isMyPosition]];
  else if (row == CellRowSet)
    return [PlacePageEditCell cellHeightWithTextValue:self.setName viewWidth:tableView.width];
  else if (row == CellRowBookmarkDescription)
  {
    if ([self descriptionIsHTML])
    {
      if (self.bookmarkDescriptionView.isLoading)
        return 0;
      else
        return [PlacePageBookmarkDescriptionCell cellHeightWithWebViewHeight:self.bookmarkDescriptionView.height];
    }
    else
    {
      return [PlacePageEditCell cellHeightWithTextValue:self.bookmarkDescription viewWidth:tableView.width];
    }
  }
  else if (row == CellRowShare)
    return [PlacePageShareCell cellHeight];
  else if (row == CellRowRouting)
    return [PlacePageRoutingCell cellHeight];
  return 0;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  CellRow row = [self cellRowForIndexPath:indexPath];
  [tableView deselectRowAtIndexPath:indexPath animated:YES];
  if (row == CellRowSet)
    [self.delegate placePageView:self willEditProperty:@"Set" inBookmarkAndCategory:GetFramework().FindBookmark([self userMark])];
  else if (row == CellRowBookmarkDescription)
    [self.delegate placePageView:self willEditProperty:@"Description" inBookmarkAndCategory:GetFramework().FindBookmark([self userMark])];
}

#define TITLE_LABEL_LEFT_OFFSET 12
#define TYPES_LABEL_LANDSCAPE_RIGHT_OFFSET 74

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
  webViewIsLoaded = YES;
  if (updatingTable)
    needToUpdateWebView = YES;
  else
    [self updateWebView];
}

- (void)updateWebView
{
  [self.bookmarkDescriptionView sizeToFit];
  [self layoutSubviews];
}

- (void)reloadHeader
{
  self.titleLabel.text = self.title;

  if ([self isMyPosition])
    self.typeLabel.text = [[MapsAppDelegate theApp].m_locationManager formattedSpeedAndAltitude:m_hasSpeed];
  else
    self.typeLabel.text = self.types;

  self.bookmarkButton.midX = self.headerView.width - 23.5;
  self.titleLabel.minX = TITLE_LABEL_LEFT_OFFSET;
  CGSize titleSize;
  CGSize typesSize;
  if ([self iPhoneInLandscape])
  {
    [self iphoneLandscapeTitleSize:&titleSize typesSize:&typesSize];
    self.titleLabel.size = titleSize;
    self.titleLabel.minY = 25.5;
    self.typeLabel.textAlignment = NSTextAlignmentRight;
    self.typeLabel.size = typesSize;
    self.typeLabel.minY = self.titleLabel.minY + 3.5;
    self.typeLabel.maxX = self.width - TYPES_LABEL_LANDSCAPE_RIGHT_OFFSET;
    self.bookmarkButton.midY = 37;
  }
  else
  {
    [self portraitTitleSize:&titleSize typesSize:&typesSize];
    self.titleLabel.size = titleSize;
    self.titleLabel.minY = 24.5;
    self.typeLabel.textAlignment = NSTextAlignmentLeft;
    self.typeLabel.size = typesSize;
    self.typeLabel.origin = CGPointMake(self.titleLabel.minX, self.titleLabel.maxY + 1);
    self.bookmarkButton.midY = 36;
  }
}

- (CGFloat)headerHeight
{
  CGSize titleSize;
  CGSize typesSize;
  if ([self iPhoneInLandscape])
  {
    [self iphoneLandscapeTitleSize:&titleSize typesSize:&typesSize];
    return MAX(titleSize.height, typesSize.height) + 34;
  }
  else
  {
    [self portraitTitleSize:&titleSize typesSize:&typesSize];
    return titleSize.height + typesSize.height + 30;
  }
}

- (void)portraitTitleSize:(CGSize *)titleSize typesSize:(CGSize *)typesSize
{
  CGFloat const maxHeight = 200;
  CGFloat const width = self.headerView.width - 90;
  *titleSize = [self.title sizeWithDrawSize:CGSizeMake(width, maxHeight) font:self.titleLabel.font];
  *typesSize = [self.types sizeWithDrawSize:CGSizeMake(width, maxHeight) font:self.typeLabel.font];
}

- (void)iphoneLandscapeTitleSize:(CGSize *)titleSize typesSize:(CGSize *)typesSize
{
  CGFloat const maxHeight = 100;
  CGFloat const width = self.headerView.width - TITLE_LABEL_LEFT_OFFSET - TYPES_LABEL_LANDSCAPE_RIGHT_OFFSET;
  *typesSize = [self.types sizeWithDrawSize:CGSizeMake(width, maxHeight) font:self.typeLabel.font];
  *titleSize = [self.title sizeWithDrawSize:CGSizeMake(width - typesSize->width - 8, maxHeight) font:self.titleLabel.font];
}

- (BOOL)iPhoneInLandscape
{
  return self.superview.width > self.superview.height && !IPAD;
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
  [self.tableView setContentOffset:CGPointZero animated:animated];

  LocationManager * locationManager = [MapsAppDelegate theApp].m_locationManager;
  if (state == PlacePageStateHidden)
    [locationManager stop:self];
  else
  {
    if ([locationManager enabledOnMap])
      [locationManager start:self];
    if (state == PlacePageStateOpened)
      [locationManager triggerCompass];
  }
}

#define BOTTOM_SHADOW_OFFSET 18

- (void)layoutSubviews
{
  if (!updatingTable)
  {
    [self setState:self.state animated:YES withCallback:YES];
    CGFloat const headerHeight = [self headerHeight];
    self.tableView.frame = CGRectMake(0, headerHeight, self.superview.width, self.backgroundView.height - headerHeight - BOTTOM_SHADOW_OFFSET);
  }
}

- (void)updateHeight:(CGFloat)height
{
  self.height = height;
  self.backgroundView.height = height;
  self.headerView.height = [self headerHeight];
}

- (CGFloat)viewMinY
{
  return self.statusBarIncluded ? (SYSTEM_VERSION_IS_LESS_THAN(@"7") ? -20 : 0) : -20;
}

- (CGPoint)arrowCenterForHeaderWithHeight:(CGFloat)headerHeight
{
  return CGPointMake(self.width / 2, headerHeight + BOTTOM_SHADOW_OFFSET - 12.5);
}

- (void)alignAnimated:(BOOL)animated
{
  UIViewAnimationOptions options = UIViewAnimationOptionCurveEaseInOut | UIViewAnimationOptionAllowUserInteraction;
  double damping = 0.95;
  CGFloat const headerHeight = [self headerHeight];
  if (self.state == PlacePageStatePreview)
  {
    self.tableView.alpha = 0;
    self.titleLabel.userInteractionEnabled = NO;
    [UIView animateWithDuration:(animated ? 0.2 : 0) delay:0.2 options:UIViewAnimationCurveEaseInOut animations:^{
      self.arrowImageView.alpha = 1;
    } completion:nil];
    [UIView animateWithDuration:(animated ? 0.4 : 0) delay:0 damping:damping initialVelocity:0 options:options animations:^{
      self.arrowImageView.center = [self arrowCenterForHeaderWithHeight:headerHeight];
      [self updateHeight:(headerHeight + BOTTOM_SHADOW_OFFSET)];
      self.minY = [self viewMinY];
      self.headerSeparator.alpha = 0;
    } completion:nil];
  }
  else if (self.state == PlacePageStateOpened)
  {
    self.tableView.alpha = 1;
    self.titleLabel.userInteractionEnabled = YES;
    self.headerSeparator.maxY = headerHeight;
    [UIView animateWithDuration:(animated ? 0.1 : 0) animations:^{
      self.arrowImageView.alpha = 0;
    }];
    [UIView animateWithDuration:(animated ? 0.4 : 0) delay:0 damping:damping initialVelocity:0 options:options animations:^{
      self.arrowImageView.center = [self arrowCenterForHeaderWithHeight:headerHeight];
      [self updateHeight:[self fullHeight]];
      self.minY = [self viewMinY];
      self.headerSeparator.alpha = 1;
    } completion:^(BOOL finished) {
      if (needToUpdateWebView)
      {
        needToUpdateWebView = NO;
        [self updateWebView];
      }
    }];
  }
  else if (self.state == PlacePageStateHidden)
  {
    [UIView animateWithDuration:(animated ? 0.4 : 0) delay:0 damping:damping initialVelocity:0 options:options animations:^{
      self.arrowImageView.center = [self arrowCenterForHeaderWithHeight:headerHeight];
      self.maxY = 0;
      self.tableView.alpha = 0;
    } completion:nil];
  }

  [self updateBookmarkViewsAlpha:animated];
}

- (CGFloat)fullHeight
{
  CGFloat const infoCellHeight = [PlacePageInfoCell cellHeightWithViewWidth:self.tableView.width inMyPositionMode:[self isMyPosition]];
  CGFloat fullHeight = [self headerHeight] + infoCellHeight + [PlacePageShareCell cellHeight] + (GetFramework().IsRoutingEnabled() ? [PlacePageRoutingCell cellHeight] : 0);
  if ([self isBookmark])
  {
    fullHeight += [PlacePageEditCell cellHeightWithTextValue:self.setName viewWidth:self.tableView.width];
    if ([self descriptionIsHTML])
      fullHeight += [PlacePageBookmarkDescriptionCell cellHeightWithWebViewHeight:self.bookmarkDescriptionView.height];
    else
      fullHeight += [PlacePageEditCell cellHeightWithTextValue:self.bookmarkDescription viewWidth:self.tableView.width];
  }
  fullHeight = MIN(fullHeight + 20, [self maxHeight]);

  return fullHeight;
}

- (void)updateBookmarkViewsAlpha:(BOOL)animated
{
  self.editImageView.center = CGPointMake(self.titleLabel.maxX + 14, self.titleLabel.minY + 13);
  [UIView animateWithDuration:(animated ? 0.3 : 0) delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    PlacePageInfoCell * cell = (PlacePageInfoCell *)[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:ROW_COMMON inSection:0]];
    if ([self isBookmark])
    {
      self.editImageView.alpha = (self.state == PlacePageStateOpened ? 1 : 0);
      cell.separator.alpha = 1;
      cell.selectedColorView.alpha = 1;
    }
    else
    {
      self.editImageView.alpha = 0;
      cell.separator.alpha = 0;
      cell.selectedColorView.alpha = 0;
    }
  } completion:nil];
}

- (void)updateBookmarkStateAnimated:(BOOL)animated
{
  if ([self isBookmark])
  {
    CGFloat const headerHeight = [self headerHeight];
    CGFloat const fullHeight = [self fullHeight];
    self.tableView.frame = CGRectMake(0, headerHeight, self.superview.width, fullHeight - headerHeight - BOTTOM_SHADOW_OFFSET);
  }

  [self resizeTableAnimated:animated];
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

- (BOOL)descriptionIsHTML
{
  NSString * description = self.bookmarkDescription;
  NSInteger const length = [description length];

  NSInteger openedCount = 0;
  NSInteger closedCount = 0;
  for (NSInteger i = 0; i < length; i++)
  {
    unichar const symbol = [description characterAtIndex:i];
    if (symbol == '<')
      openedCount++;
    else if (symbol == '>')
      closedCount++;
  }
  return openedCount == closedCount && openedCount != 0;
}

- (CGFloat)maxHeight
{
  if (IPAD)
    return 600;
  else
    return self.superview.width > self.superview.height ? 270 : self.superview.height - 64;
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
  if ([self isBookmark] && bookmarkAndCategory == GetFramework().FindBookmark([self userMark]))
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

- (void)routeCellDidSetEndPoint:(PlacePageRoutingCell *)cell
{
  GetFramework().SetRouteEnd([self pinPoint]);
  [self setState:PlacePageStateHidden animated:YES withCallback:YES];
}

- (void)routeCellDidSetStartPoint:(PlacePageRoutingCell *)cell
{
  GetFramework().SetRouteStart([self pinPoint]);
  [self setState:PlacePageStateHidden animated:YES withCallback:YES];
}

- (void)infoCellDidPressColorSelector:(PlacePageInfoCell *)cell
{
  UIWindow * appWindow = [[UIApplication sharedApplication].windows firstObject];
  UIWindow * window = [[appWindow subviews] firstObject];

  self.pickerView = [[UIView alloc] initWithFrame:window.bounds];
  ColorPickerView * colorPicker = [[ColorPickerView alloc] initWithWidth:(IPAD ? 340 : 300) andSelectButton:[ColorPickerView getColorIndex:[self colorName]]];
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
  ASSERT([self isBookmark], ());
  PlacePageInfoCell * cell = (PlacePageInfoCell *)[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:ROW_COMMON inSection:0]];
  UIColor * color = [ColorPickerView colorForName:[ColorPickerView colorName:colorIndex]];
  cell.color = color;
  [cell layoutSubviews];

  Framework & framework = GetFramework();
  UserMark const * mark = [self userMark];
  BookmarkData newData = static_cast<Bookmark const *>(mark)->GetData();
  newData.SetType([[ColorPickerView colorName:colorIndex] UTF8String]);
  
  BookmarkAndCategory bookmarkAndCategory = framework.FindBookmark(mark);
  framework.ReplaceBookmark(bookmarkAndCategory.first, bookmarkAndCategory.second, newData);
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
  if ([self isBookmark] && self.state == PlacePageStateOpened)
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
  _bookmarkDescription = nil;
  needUpdateAddressInfo = YES;
  needToUpdateWebView = NO;
  webViewIsLoaded = NO;
}

- (void)showUserMark:(UserMarkCopy *)mark
{
  [self clearCachedProperties];
  self.temporaryTitle = nil;

  m_mark.reset(mark);
  m_cachedMark = m_mark;

  UserMark const * innerMark = [self userMark];
  if ([self isBookmark])
    [self bookmarkActivated:static_cast<Bookmark const *>(innerMark)];
  else
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
  return m_mark ? [self userMark]->GetMarkType() == type : NO;
}

- (BOOL)isMyPosition
{
  return [self isMarkOfType:UserMark::MY_POSITION];
}

- (BOOL)isBookmark
{
  return [self isMarkOfType:UserMark::BOOKMARK];
}

- (m2::PointD)pinPoint
{
  return m_mark ? [self userMark]->GetOrg() : m2::PointD();
}

- (NSString *)nonEmptyTitle:(string const &)name
{
  return name.empty() ? NSLocalizedString(@"dropped_pin", nil) : [NSString stringWithUTF8String:name.c_str()];
}

- (NSString *)newBookmarkName
{
  if ([self isMyPosition])
  {
    Framework & f = GetFramework();
    search::AddressInfo info;
    m2::PointD pxPivot;
    if (f.GetVisiblePOI(f.GtoP([self pinPoint]), pxPivot, info))
    {
      return [self nonEmptyTitle:info.GetPinName()];
    }
    else
    {
      self.temporaryTitle = NSLocalizedString(@"my_position", nil);
      return [self nonEmptyTitle:[self addressInfo].GetPinName()];
    }
  }
  else
  {
    return self.title;
  }
}

- (NSString *)title
{
  if (!_title)
  {
    if ([self isBookmark])
    {
      Bookmark const * bookmark = static_cast<Bookmark const *>([self userMark]);
      _title = [self nonEmptyTitle:bookmark->GetName()];
    }
    else if ([self isMarkOfType:UserMark::API])
    {
      ApiMarkPoint const * apiMark = static_cast<ApiMarkPoint const *>([self userMark]);
      _title = [self nonEmptyTitle:apiMark->GetName()];
    }
    else
    {
      if ([NSString instancesRespondToSelector:@selector(capitalizedStringWithLocale:)]) // iOS 6 and higher
        _title = [[self nonEmptyTitle:[self addressInfo].GetPinName()] capitalizedStringWithLocale:[NSLocale currentLocale]];
      else // iOS 5 
        _title = [[self nonEmptyTitle:[self addressInfo].GetPinName()] capitalizedString];
    }

    NSString * droppedPinTitle = NSLocalizedString(@"dropped_pin", nil);
    if ([_title isEqualToString:droppedPinTitle])
      self.temporaryTitle = droppedPinTitle;
  }
  return _title;
}

- (NSString *)types
{
  if (!_types)
  {
    if ([self isMyPosition])
    {
      _types = [[MapsAppDelegate theApp].m_locationManager formattedSpeedAndAltitude:m_hasSpeed];
    }
    else
    {
      NSString * types = [NSString stringWithUTF8String:[self addressInfo].GetPinType().c_str()];
      if ([NSString instancesRespondToSelector:@selector(capitalizedStringWithLocale:)]) // iOS 6 and higher
        _types = [types capitalizedStringWithLocale:[NSLocale currentLocale]];
      else // iOS 5
        _types = [types capitalizedString];
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

- (NSString *)bookmarkDescription
{
  if (!_bookmarkDescription)
  {
    if ([self isBookmark])
    {
      Bookmark const * bookmark = static_cast<Bookmark const *>([self userMark]);
      std::string const & description = bookmark->GetDescription();
      _bookmarkDescription = description.empty() ? NSLocalizedString(@"description", nil) : [NSString stringWithUTF8String:description.c_str()];
    }
    else
    {
      _bookmarkDescription = @"";
    }
  }
  return _bookmarkDescription;
}

- (NSString *)address
{
  return @""; // because we don't use address

  if (!_address)
  {
    std::string const & address = [self addressInfo].FormatAddress();
    _address = address.empty() ? @"" : [NSString stringWithUTF8String:address.c_str()];
  }
  return _address;
}

- (NSString *)colorName
{
  if ([self isBookmark])
  {
    Bookmark const * bookmark = static_cast<Bookmark const *>([self userMark]);
    std::string const & type = bookmark->GetType();
    return type.empty() ? @"placemark-red" : [NSString stringWithUTF8String:type.c_str()];
  }
  else
  {
    return @"placemark-red";
  }
}

- (search::AddressInfo)addressInfo
{
  if (needUpdateAddressInfo)
  {
    needUpdateAddressInfo = NO;
    if ([self isMarkOfType:UserMark::POI] || [self isMarkOfType:UserMark::SEARCH] || [self isMyPosition])
    {
      SearchMarkPoint const * mark = static_cast<SearchMarkPoint const *>([self userMark]);
      m_addressInfo = mark->GetInfo();
    }
    else if (m_mark)
    {
      search::AddressInfo addressInfo;
      GetFramework().GetAddressInfoForGlobalPoint([self userMark]->GetOrg(), addressInfo);
      m_addressInfo = addressInfo;
    }
    else
    {
      m_addressInfo = search::AddressInfo();
    }
  }
  return m_addressInfo;
}

- (UserMark const *)userMark
{
  return m_mark ? m_mark->GetUserMark() : NULL;
}

- (shared_ptr<UserMarkCopy>)cachedMark
{
  return m_cachedMark ? m_cachedMark : shared_ptr<UserMarkCopy>(GetFramework().GetAddressMark([self pinPoint])->Copy());
}

- (void)deleteBookmark
{
  Framework & framework = GetFramework();
  UserMark const * mark = [self userMark];
  BookmarkAndCategory const & bookmarkAndCategory = framework.FindBookmark(mark);
  BookmarkCategory * category = framework.GetBmCategory(bookmarkAndCategory.first);
  m_mark = [self cachedMark];
  framework.ActivateUserMark([self userMark]);
  if (category)
  {
    category->DeleteBookmark(bookmarkAndCategory.second);
    category->SaveToKMLFile();
  }
  framework.Invalidate();

  _title = nil;
  _types = nil;
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
      m_mark.reset(category->GetBookmark(bookmarkIndex)->Copy());
    }
    else
    {
      size_t const categoryIndex = framework.LastEditedBMCategory();
      BookmarkData data = BookmarkData([[self newBookmarkName] UTF8String], framework.LastEditedBMType());
      size_t const bookmarkIndex = framework.AddBookmark(categoryIndex, [self pinPoint], data);
      BookmarkCategory const * category = framework.GetBmCategory(categoryIndex);
      m_mark.reset(category->GetBookmark(bookmarkIndex)->Copy());
    }
    framework.ActivateUserMark([self userMark]);
    framework.Invalidate();

    _title = nil;
    _types = nil;
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
  [self.delegate placePageView:self willShareText:self.title point:[self pinPoint]];
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
  UIScrollView * sv = scrollView;
  if (sv.contentOffset.y + sv.height - sv.contentSize.height > 30 && !sv.dragging && sv.decelerating)
  {
    if (self.state == PlacePageStateOpened)
      [self setState:PlacePageStateHidden animated:YES withCallback:YES];
  }
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
  if (buttonIndex == alertView.cancelButtonIndex)
  {
    [[Statistics instance] logProposalReason:@"Add Bookmark" withAnswer:@"NO"];
  }
  else
  {
    [[Statistics instance] logProposalReason:@"Add Bookmark" withAnswer:@"YES"];
    [[UIApplication sharedApplication] openProVersionFrom:@"ios_pp_bookmark"];
  }
}

- (CopyLabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[CopyLabel alloc] initWithFrame:CGRectZero];
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.font = [UIFont fontWithName:@"HelveticaNeue" size:19];
    _titleLabel.textColor = [UIColor blackColor];
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
    _typeLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:14];
    _typeLabel.textAlignment = NSTextAlignmentLeft;
    _typeLabel.textColor = [UIColor colorWithColorCode:@"565656"];
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
    _headerView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, self.width, 0)];
    _headerView.backgroundColor = [UIColor clearColor];
    _headerView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleBottomMargin;

    UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap:)];
    if (SYSTEM_VERSION_IS_LESS_THAN(@"6"))
    {
      UIView * touchView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, _headerView.width - 64, _headerView.height)];
      [_headerView addSubview:touchView];
      [touchView addGestureRecognizer:tap];
    }
    else
    {
      [_headerView addGestureRecognizer:tap];
    }
    [_headerView addSubview:self.titleLabel];
    [_headerView addSubview:self.typeLabel];
    [_headerView addSubview:self.bookmarkButton];

    UIImage * separatorImage = [[UIImage imageNamed:@"PlacePageSeparator"] resizableImageWithCapInsets:UIEdgeInsetsZero];
    CGFloat const offset = 12.5;
    UIImageView * separator = [[UIImageView alloc] initWithFrame:CGRectMake(offset, _headerView.height - separatorImage.size.height, _headerView.width - 2 * offset, separatorImage.size.height)];
    separator.image = separatorImage;
    separator.maxY = _headerView.height;
    separator.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    [_headerView addSubview:separator];
    self.headerSeparator = separator;

    [_headerView addSubview:self.editImageView];

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

- (UIImageView *)backgroundView
{
  if (!_backgroundView)
  {
    _backgroundView = [[UIImageView alloc] initWithFrame:self.bounds];
    _backgroundView.image = [[UIImage imageNamed:@"PlacePageBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(0, 0, 18, 0)];
    _backgroundView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  }
  return _backgroundView;
}

- (UIImageView *)arrowImageView
{
  if (!_arrowImageView)
  {
    _arrowImageView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 50, 40)];
    _arrowImageView.image = [UIImage imageNamed:@"PlacePageArrow"];
    _arrowImageView.userInteractionEnabled = YES;
    UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap:)];
    [_arrowImageView addGestureRecognizer:tap];
  }
  return _arrowImageView;
}

- (UIWebView *)bookmarkDescriptionView
{
  if (!_bookmarkDescriptionView)
  {
    _bookmarkDescriptionView = [[UIWebView alloc] initWithFrame:CGRectZero];
    _bookmarkDescriptionView.delegate = self;
  }
  return _bookmarkDescriptionView;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end
