
#import "PlacePageView.h"
#import "UIKitCategories.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "BookmarksRootVC.h"
#import "Framework.h"
#import "ContextViews.h"
#include "../../search/result.hpp"

@interface PlacePageView () <LocationObserver, UIGestureRecognizerDelegate>

@property (nonatomic) CopyLabel * titleLabel;
@property (nonatomic) UIButton * bookmarkButton;
@property (nonatomic) UILabel * typeLabel;
@property (nonatomic) UIView * lineView;
@property (nonatomic) CopyLabel * addressLabel;
@property (nonatomic) LocationImageView * locationView;
@property (nonatomic) UIButton * shareButton;
@property (nonatomic) UIButton * editButton;
@property (nonatomic) UIButton * largeShareButton;
@property (nonatomic) UIScrollView * scrollView;

@property (nonatomic) UIButton * guideButton;
@property (nonatomic) UIImageView * guideImageView;
@property (nonatomic) UILabel * guideLeftLabel;
@property (nonatomic) UILabel * guideRightLabel;

@property (nonatomic) CGFloat pageShift;
@property (nonatomic) CGFloat maximumPageShift;
@property (nonatomic) CGFloat startPageShift;

@end

@implementation PlacePageView
{
  CGFloat locationMidY;
  CGFloat addressMidY;
  CGFloat shareMidY;

  UserMark const * m_mark;
}

-(BOOL)isMarkOfType:(UserMark::Type)type
{
  ASSERT(m_mark != NULL, ());
  ASSERT(m_mark->GetContainer() != NULL, ());
  return m_mark->GetMarkType() == type;
}

-(BOOL)isBookmark
{
  return [self isMarkOfType:UserMark::BOOKMARK];
}

-(BOOL)isApiPoint
{
  return [self isMarkOfType:UserMark::API];
}

-(BOOL)isEmpty
{
  return m_mark == NULL;
}

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  self.backgroundColor = [UIColor applicationBackgroundColor];
  self.clipsToBounds = YES;
  m_mark = NULL;

  UIView * tapView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, self.width, 56)];
  tapView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  [self addSubview:tapView];
  UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap:)];
  [tapView addGestureRecognizer:tap];

  UIPanGestureRecognizer * pan = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(pan:)];
  [self addGestureRecognizer:pan];

  UISwipeGestureRecognizer * swipeDown = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipe:)];
  swipeDown.direction = UISwipeGestureRecognizerDirectionDown;
  [self addGestureRecognizer:swipeDown];

  [self addSubview:self.titleLabel];
  [self addSubview:self.typeLabel];
  [self addSubview:self.bookmarkButton];
  [self addSubview:self.lineView];
  [self addSubview:self.scrollView];
  [self.scrollView addSubview:self.addressLabel];
  [self.scrollView addSubview:self.locationView];
  [self.scrollView addSubview:self.shareButton];
  [self.scrollView addSubview:self.guideButton];
  [self.scrollView addSubview:self.editButton];
  [self.scrollView addSubview:self.largeShareButton];

  [self layoutShareAndEditButtons];
  [self setState:PlacePageStateHidden animated:NO];

  UILongPressGestureRecognizer * locationPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(locationPress:)];
  [self.locationView addGestureRecognizer:locationPress];
  UITapGestureRecognizer * locationTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(hideMenuTap:)];
  [self.locationView addGestureRecognizer:locationTap];
  UITapGestureRecognizer * addressTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(hideMenuTap:)];
  [self.addressLabel addGestureRecognizer:addressTap];

  UILongPressGestureRecognizer * titlePress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(labelPress:)];
  [self.titleLabel addGestureRecognizer:titlePress];
  UITapGestureRecognizer * titleTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap:)];
  [self.titleLabel addGestureRecognizer:titleTap];

  UILongPressGestureRecognizer * addressPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(labelPress:)];
  [self.addressLabel addGestureRecognizer:addressPress];

  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(bookmarkDeletedNotification:) name:BOOKMARK_DELETED_NOTIFICATION object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(bookmarkCategoryDeletedNotification:) name:BOOKMARK_CATEGORY_DELETED_NOTIFICATION object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(metricsChangedNotification:) name:METRICS_CHANGED_NOTIFICATION object:nil];

  return self;
}

- (void)layoutShareAndEditButtons
{
  CGFloat const borderOffset = 21;
  CGFloat const betweenOffset = 4;
  CGFloat const width = (self.width - 2 * borderOffset - betweenOffset) / 2;

  self.shareButton.width = width;
  self.shareButton.minX = borderOffset;

  self.editButton.width = width;
  self.editButton.minX = self.shareButton.maxX + betweenOffset;

  self.largeShareButton.width = self.width - 2 * borderOffset;
  self.largeShareButton.midX = self.width / 2;
}

- (void)metricsChangedNotification:(NSNotification *)notification
{
  self.locationView.pinPoint = m_mark->GetOrg();
}

- (void)bookmarkCategoryDeletedNotification:(NSNotification *)notification
{
  BookmarkAndCategory bmAndCat = GetFramework().FindBookmark(m_mark);
  if (bmAndCat.first == [[notification object] integerValue])
  {
    [self deleteBookmark];
    [self updateBookmarkStateAnimated:NO];
  }
}

- (void)bookmarkDeletedNotification:(NSNotification *)notification
{
  BookmarkAndCategory bmAndCat = GetFramework().FindBookmark(m_mark);
  BookmarkAndCategory bookmarkAndCategory;
  [[notification object] getValue:&bookmarkAndCategory];
  if (bookmarkAndCategory == bmAndCat)
  {
    [self deleteBookmark];
    [self updateBookmarkStateAnimated:NO];
  }
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {}

- (void)setState:(PlacePageState)state animated:(BOOL)animated
{
  [self setState:state duration:(animated ? 0.5 : 0)];
}

- (void)setState:(PlacePageState)state duration:(NSTimeInterval)duration
{
  [UIView animateWithDuration:duration delay:0 damping:0.75 initialVelocity:0 options:(UIViewAnimationOptionCurveEaseInOut | UIViewAnimationOptionAllowUserInteraction) animations:^{
    if (state == PlacePageStateHidden)
      self.pageShift = 0;
    else if (state == PlacePageStateBitShown)
      self.pageShift = self.lineView.minY;
    else if (state == PlacePageStateOpened)
      self.pageShift = self.maximumPageShift;
  } completion:nil];
  [UIView animateWithDuration:(duration + 0.15) delay:0 damping:0.8 initialVelocity:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    if (state == PlacePageStateBitShown)
    {
      self.locationView.midY = locationMidY + 40;
      self.addressLabel.midY = addressMidY + 32;
      self.shareButton.midY = shareMidY + 26;
      self.editButton.midY = self.shareButton.midY;
      self.largeShareButton.midY = self.shareButton.midY;
    }
    else if (state == PlacePageStateOpened)
    {
      self.locationView.midY = locationMidY;
      self.addressLabel.midY = addressMidY;
      self.shareButton.midY = shareMidY;
      self.editButton.midY = self.shareButton.midY;
      self.largeShareButton.midY = self.shareButton.midY;
    }
  } completion:nil];
  [self willChangeValueForKey:@"state"];
  _state = state;
  [self didChangeValueForKey:@"state"];
}

- (void)setPageShift:(CGFloat)pageShift
{
  CGFloat delta = pageShift - self.maximumPageShift;
  if (delta > 0)
  {
    delta = MIN(50, powf(delta, 0.65));
    pageShift = self.maximumPageShift + delta;
  }
  pageShift = MAX(0, pageShift);
  self.minY = self.superview.height - pageShift;

  _pageShift = pageShift;
}

- (void)swipe:(UISwipeGestureRecognizer *)sender
{
  if (sender.direction == UISwipeGestureRecognizerDirectionDown && self.state == PlacePageStateBitShown)
    [self setState:PlacePageStateHidden animated:YES];
}

- (void)labelPress:(UILongPressGestureRecognizer *)sender
{
  UIMenuController * menuController = [UIMenuController sharedMenuController];
  if (!menuController.isMenuVisible)
  {
    CGPoint tapPoint = [sender locationInView:sender.view.superview];
    [menuController setTargetRect:CGRectMake(tapPoint.x, sender.view.minY, 0, 0) inView:sender.view.superview];
    [menuController setMenuVisible:YES animated:YES];
    [sender.view becomeFirstResponder];
    [menuController update];
  }
}

- (void)locationPress:(UILongPressGestureRecognizer *)sender
{
  UIMenuController * menuController = [UIMenuController sharedMenuController];
  if (!menuController.isMenuVisible)
  {
    UIMenuItem * item1 = [[UIMenuItem alloc] initWithTitle:NSLocalizedString(@"copy_degrees", nil) action:@selector(copyDegreesLocation:)];
    UIMenuItem * item2 = [[UIMenuItem alloc] initWithTitle:NSLocalizedString(@"copy_decimal", nil) action:@selector(copyDecimalLocation:)];
    menuController.menuItems = @[item1, item2];
    CGPoint tapPoint = [sender locationInView:sender.view.superview];
    [menuController setTargetRect:CGRectMake(tapPoint.x, sender.view.minY, 0, 0) inView:sender.view.superview];
    [menuController setMenuVisible:YES animated:YES];
    [sender.view becomeFirstResponder];
    [menuController update];
  }
}

- (void)tap:(UITapGestureRecognizer *)sender
{
  [[UIMenuController sharedMenuController] setMenuVisible:NO animated:YES];
  if (self.state == PlacePageStateBitShown)
    [self setState:PlacePageStateOpened animated:YES];
  else if (self.state == PlacePageStateOpened)
    [self setState:PlacePageStateBitShown animated:YES];
}

- (void)hideMenuTap:(UITapGestureRecognizer *)sender
{
  [[UIMenuController sharedMenuController] setMenuVisible:NO animated:YES];
}

- (void)pan:(UIPanGestureRecognizer *)sender
{
  if (sender.state == UIGestureRecognizerStateBegan)
  {
    self.startPageShift = self.pageShift;
    self.locationView.midY = locationMidY;
    self.addressLabel.midY = addressMidY;
    self.shareButton.midY = shareMidY;
    self.editButton.midY = self.shareButton.midY;
    self.largeShareButton.midY = self.shareButton.midY;
  }

  self.pageShift = self.startPageShift - [sender translationInView:self.superview].y;

  if (sender.state == UIGestureRecognizerStateCancelled || sender.state == UIGestureRecognizerStateEnded)
  {
    CGFloat velocity = [sender velocityInView:self.superview].y;
    CGFloat const minV = 1000;
    CGFloat const maxV = 5000;
    BOOL isPositive = velocity > 0;
    CGFloat absV = ABS(velocity);
    absV = MIN(maxV, MAX(minV, absV));

    PlacePageState state;
    if (isPositive)
      state = self.pageShift < (self.lineView.midY - 1) ? PlacePageStateHidden : PlacePageStateBitShown;
    else
      state = PlacePageStateOpened;
    NSTimeInterval duration = 500 / absV;

    [self setState:state duration:duration];
  }
}

- (void)layoutSubviews
{
  [self updateHeight];

  if (self.state == PlacePageStateOpened)
    self.pageShift = self.maximumPageShift;
}

- (void)updateHeight
{
  if ([self isEmpty] == YES)
  {
    self.height = 0;
    return;
  }

  if (!self.titleLabel.hidden)
  {
    [self.titleLabel sizeToFit];
    self.titleLabel.size = CGSizeMake(MIN(self.titleLabel.width, self.width - 70), 22);
    self.titleLabel.origin = CGPointMake(23, 9);
  }

  if (!self.typeLabel.hidden)
  {
    [self.typeLabel sizeToFit];
    self.typeLabel.width += 4;
    self.typeLabel.height += 2;
    self.typeLabel.origin = CGPointMake(self.titleLabel.minX + 1, self.titleLabel.maxY + 1);
  }

  self.bookmarkButton.hidden = NO;
  self.bookmarkButton.center = CGPointMake(self.width - 32, 28);

  CGFloat lineOffsetX = 10;
  self.lineView.size = CGSizeMake(self.width - 2 * lineOffsetX, 0.5);
  self.lineView.origin = CGPointMake(lineOffsetX, 56);
  self.lineView.hidden = NO;

  CGFloat shift = 12;

  if (!self.addressLabel.hidden)
  {
    CGFloat const offsetX = 22;
    self.addressLabel.width = self.width - 2 * offsetX;
    [self.addressLabel sizeToFit];
    self.addressLabel.origin = CGPointMake(offsetX, shift);
    shift = self.addressLabel.maxY;
    addressMidY = self.addressLabel.midY;
  }

  if (!self.locationView.hidden)
  {
    self.locationView.midX = self.width / 2;
    self.locationView.minY = shift + 15;
    shift = self.locationView.maxY;
    locationMidY = self.locationView.midY;
  }

  if ([self isApiPoint] == YES)
  {
    self.guideButton.hidden = NO;
    self.guideButton.midX = self.width / 2;
    self.guideButton.minY = shift + 9;
    shift = self.guideButton.maxY;
  }

  self.editButton.hidden = NO;
  self.shareButton.hidden = NO;
  self.largeShareButton.hidden = NO;
  self.shareButton.minY = shift + 9;
  self.largeShareButton.minY = self.shareButton.minY;
  self.editButton.minY = self.shareButton.minY;
  shareMidY = self.shareButton.midY;

  shift = self.shareButton.maxY + 14;
  CGFloat maxHeight = self.superview.height - 68;
  self.maximumPageShift = MIN(shift + self.lineView.maxY, maxHeight);
  self.height = self.maximumPageShift + 50;
  self.scrollView.frame = CGRectMake(0, self.lineView.maxY, self.width, self.height - self.lineView.maxY);
  self.scrollView.contentInset = UIEdgeInsetsMake(0, 0, self.height - self.maximumPageShift, 0);
  self.scrollView.contentSize = CGSizeMake(self.scrollView.width, shift);
}

- (void)hideAll
{
  self.typeLabel.hidden = YES;
  [self.scrollView.subviews makeObjectsPerformSelector:@selector(setHidden:) withObject:@YES];
}

- (void)editButtonPressed:(id)sender
{
  if ([self isBookmark] == YES)
    [self.delegate placePageView:self willEditBookmarkAndCategory:GetFramework().FindBookmark(m_mark)];
  else
  {
    search::AddressInfo info;
    GetFramework().GetAddressInfoForGlobalPoint(m_mark->GetOrg(), info);
    [self.delegate placePageView:self willEditBookmarkWithInfo:info point:m_mark->GetOrg()];
  }
}

- (void)shareButtonPressed:(id)sender
{
  search::AddressInfo info;
  GetFramework().GetAddressInfoForGlobalPoint(m_mark->GetOrg(), info);
  [self.delegate placePageView:self willShareInfo:info point:m_mark->GetOrg()];
}

- (void)bookmarkButtonPressed:(UIButton *)sender
{
  if (self.isBookmark)
    [self deleteBookmark];
  else
    [self addBookmark];
  [self updateBookmarkStateAnimated:YES];
}

- (void)guideButtonPressed:(id)sender
{
  NSString * urlString = [NSString stringWithUTF8String:GetFramework().GetApiDataHolder().GetGlobalBackUrl().c_str()];
  [[UIApplication sharedApplication] openURL:[NSURL URLWithString:urlString]];
}

namespace
{
  template <class T>
  T const * CastData(UserMark const * data)
  {
    ASSERT(dynamic_cast<T const *>(data) != NULL, ());
    return static_cast<T const *>(data);
  }
}

- (void)showUserMark:(UserMark const *)mark
{
  m_mark = mark;
  UserMark::Type type = mark->GetMarkType();
  switch (type)
  {
    case UserMark::BOOKMARK:
      [self bookmarkActivated:CastData<Bookmark>(m_mark)];
      break;
    case UserMark::POI:
      [self poiActivated:CastData<PoiMarkPoint>(m_mark)];
      break;
    case UserMark::API:
      [self apiPointActivated:CastData<ApiMarkPoint>(m_mark)];
      break;
    case UserMark::SEARCH:
      [self searchResultActivated:CastData<SearchMarkPoint>(m_mark)];
      break;
      
    default:
      break;
  }
}

- (void)poiActivated:(PoiMarkPoint const *)data
{
  search::AddressInfo info = data->GetInfo();
  [self processName:info.GetPinName() type:info.GetPinType() address:info.FormatAddress()];
}

- (void)searchResultActivated:(SearchMarkPoint const *)data
{
  [[MapsAppDelegate theApp].m_locationManager start:self];
  [self hideAll];
  
  search::AddressInfo info = data->GetInfo();
  string name = info.GetPinName();
  self.titleLabel.text = name.empty() ? NSLocalizedString(@"dropped_pin", nil) : [NSString stringWithUTF8String:name.c_str()];
  self.titleLabel.hidden = NO;
  
  if (!info.GetPinType().empty())
  {
    self.typeLabel.text = [NSString stringWithUTF8String:info.GetPinType().c_str()];
    self.typeLabel.hidden = NO;
  }
  
  self.addressLabel.text = [NSString stringWithUTF8String:info.FormatAddress().c_str()];
  self.addressLabel.hidden = ![self.addressLabel.text length];
  
  self.locationView.hidden = NO;
  self.locationView.pinPoint = m_mark->GetOrg();
  
  [self updateHeight];
  [self updateBookmarkStateAnimated:NO];
}

- (void)apiPointActivated:(ApiMarkPoint const *)data
{
  Framework & framework = GetFramework();
  
  NSString * appTitle = [NSString stringWithUTF8String:framework.GetApiDataHolder().GetAppTitle().c_str()];
  if ([appTitle isEqualToString:@"GuideWithMe"])
  {
    NSString * urlString = [NSString stringWithUTF8String:framework.GetApiDataHolder().GetGlobalBackUrl().c_str()];
    NSString * lastComponent = [urlString componentsSeparatedByString:@"-"][1];
    NSString * imageName = [[lastComponent substringWithRange:NSMakeRange(0, [lastComponent length] - 3)] lowercaseString];
    
    self.guideLeftLabel.text = NSLocalizedString(@"more_info", nil);
    [self.guideLeftLabel sizeToFit];
    self.guideRightLabel.text = appTitle;
    [self.guideRightLabel sizeToFit];
    self.guideImageView.image = [self iconImageWithImage:[UIImage imageNamed:imageName]];
    
    CGFloat const betweenSpace = 4;
    CGFloat const width = self.guideLeftLabel.width + self.guideImageView.width + self.guideRightLabel.width + 2 * betweenSpace;
    UIView * contentView = self.guideImageView.superview;
    CGFloat const startX = (contentView.width - width) / 2;
    self.guideLeftLabel.minX = startX;
    self.guideImageView.minX = self.guideLeftLabel.maxX + betweenSpace;
    self.guideRightLabel.minX = self.guideImageView.maxX + betweenSpace;
    
    self.guideLeftLabel.midY = contentView.height / 2;
    self.guideImageView.midY = contentView.height / 2;
    self.guideRightLabel.midY = contentView.height / 2;
    
    [self.guideButton setTitle:nil forState:UIControlStateNormal];
  }
  else
  {
    self.guideLeftLabel.text = nil;
    self.guideRightLabel.text = nil;
    self.guideImageView.image = nil;
    [self.guideButton setTitle:appTitle forState:UIControlStateNormal];
  }
  
  self.titleLabel.text = [NSString stringWithUTF8String:data->GetName().c_str()];
  
  search::AddressInfo info;
  framework.GetAddressInfoForGlobalPoint(m_mark->GetOrg(), info);
  [self processName:info.GetPinName() type:info.GetPinType() address:info.FormatAddress()];
}

- (void)bookmarkActivated:(Bookmark const *)data
{
  Framework & framework = GetFramework();
  [[MapsAppDelegate theApp].m_locationManager start:self];
  [self hideAll];
  
  string name = data->GetName();
  self.titleLabel.text = name.empty() ? NSLocalizedString(@"dropped_pin", nil) : [NSString stringWithUTF8String:name.c_str()];
  self.titleLabel.hidden = NO;
  
  if (!data->GetType().empty())
  {
    self.typeLabel.text = [NSString stringWithUTF8String:data->GetType().c_str()];
    self.typeLabel.hidden = NO;
  }
  
  search::AddressInfo addressInfo;
  framework.GetAddressInfoForGlobalPoint(m_mark->GetOrg(), addressInfo);
  self.addressLabel.text = [NSString stringWithUTF8String:addressInfo.FormatAddress().c_str()];
  self.addressLabel.hidden = ![self.addressLabel.text length];
  
  self.locationView.hidden = NO;
  self.locationView.pinPoint = m_mark->GetOrg();
  
  [self updateHeight];
  [self updateBookmarkStateAnimated:NO];
}

- (void)processName:(string const &)name type:(string const &)type address:(string const &)address
{
  [[MapsAppDelegate theApp].m_locationManager start:self];
  [self hideAll];

  if ([self isApiPoint] == NO)
    self.titleLabel.text = name.empty() ? NSLocalizedString(@"dropped_pin", nil) : [NSString stringWithUTF8String:name.c_str()];
  self.titleLabel.hidden = NO;

  if (!type.empty())
  {
    self.typeLabel.text = [NSString stringWithUTF8String:type.c_str()];
    self.typeLabel.hidden = NO;
  }

  self.addressLabel.text = [NSString stringWithUTF8String:address.c_str()];
  self.addressLabel.hidden = ![self.addressLabel.text length];

  self.locationView.hidden = NO;
  self.locationView.pinPoint = m_mark->GetOrg();

  [self updateHeight];
  [self updateBookmarkStateAnimated:NO];
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
  ASSERT(m_mark->GetMarkType() == UserMark::BOOKMARK, ());
  ASSERT(m_mark->GetContainer() != NULL, ());
  ASSERT(m_mark->GetContainer()->GetType() == UserMarkContainer::BOOKMARK_MARK, ());
  Framework & fm = GetFramework();
  BookmarkAndCategory bmAndCat = fm.FindBookmark(m_mark);
  if (IsValid(bmAndCat))
  {
    m2::PointD orgPt = m_mark->GetOrg();
    fm.GetBmCategory(bmAndCat.first)->DeleteBookmark(bmAndCat.second);
    m_mark = fm.ActivateAddressMark(orgPt);
    ASSERT(m_mark != NULL, ());
  }
  
  fm.Invalidate();
}

- (void)addBookmark
{
  Framework & framework = GetFramework();
  
  search::AddressInfo info;
  framework.GetAddressInfoForGlobalPoint(m_mark->GetOrg(), info);
  
  size_t categoryIndex = framework.LastEditedBMCategory();
  string boomarkType = framework.LastEditedBMType();
  
  string pinName = info.GetPinName();
  string name = pinName.empty() ? [NSLocalizedString(@"dropped_pin", nil) UTF8String] : pinName.c_str();
  BookmarkData bm(name, boomarkType);
  size_t bmIndex = framework.AddBookmark(categoryIndex, m_mark->GetOrg(), bm);
  m_mark = framework.GetBmCategory(categoryIndex)->GetBookmark(bmIndex);
  framework.Invalidate();
}

- (void)updateBookmarkStateAnimated:(BOOL)animated
{
  bool isBookmark = [self isBookmark];
  self.bookmarkButton.selected = isBookmark;
  [UIView animateWithDuration:0.25 animations:^{
    self.largeShareButton.alpha = isBookmark ? 0 : 1;
  }];
}

- (void)onLocationError:(location::TLocationError)errorCode
{
  NSLog(@"Location error in PlacePageView");
}

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  CGFloat oldHeight = self.locationView.height;
  self.locationView.userLocation = [MapsAppDelegate theApp].m_locationManager.lastLocation;
  if (oldHeight != self.locationView.height)
  {
    [self updateHeight];
    [self setState:self.state animated:YES];
  }
}

- (CopyLabel *)titleLabel
{
  if (!_titleLabel)
  {
    _titleLabel = [[CopyLabel alloc] initWithFrame:CGRectZero];
    _titleLabel.backgroundColor = [UIColor clearColor];
    _titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:17.5];
    _titleLabel.textColor = [UIColor colorWithColorCode:@"333333"];
    _titleLabel.autoresizingMask = UIViewAutoresizingFlexibleRightMargin;
  }
  return _titleLabel;
}

- (UILabel *)typeLabel
{
  if (!_typeLabel)
  {
    _typeLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _typeLabel.backgroundColor = [UIColor colorWithColorCode:@"33cc66"];
    _typeLabel.font = [UIFont fontWithName:@"HelveticaNeue-Bold" size:10];
    _typeLabel.textAlignment = NSTextAlignmentCenter;
    _typeLabel.textColor = [UIColor whiteColor];
    _typeLabel.autoresizingMask = UIViewAutoresizingFlexibleRightMargin;
  }
  return _typeLabel;
}

- (UIView *)lineView
{
  if (!_lineView)
  {
    _lineView = [[UIView alloc] initWithFrame:CGRectZero];
    _lineView.backgroundColor = [UIColor colorWithColorCode:@"cccccc"];
    _lineView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  }
  return _lineView;
}

- (CopyLabel *)addressLabel
{
  if (!_addressLabel)
  {
    _addressLabel = [[CopyLabel alloc] initWithFrame:CGRectZero];
    _addressLabel.backgroundColor = [UIColor clearColor];
    _addressLabel.numberOfLines = 0;
    _addressLabel.lineBreakMode = NSLineBreakByWordWrapping;
    _addressLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:15];
    _addressLabel.textColor = [UIColor colorWithColorCode:@"333333"];
    _addressLabel.autoresizingMask = UIViewAutoresizingFlexibleRightMargin;
  }
  return _addressLabel;
}

- (UIButton *)bookmarkButton
{
  if (!_bookmarkButton)
  {
    _bookmarkButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 64, 44)];
    _bookmarkButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
    [_bookmarkButton setImage:[UIImage imageNamed:@"PlacePageBookmarkButton"] forState:UIControlStateNormal];
    [_bookmarkButton setImage:[UIImage imageNamed:@"PlacePageBookmarkButtonSelected"] forState:UIControlStateSelected];
    [_bookmarkButton addTarget:self action:@selector(bookmarkButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
  }
  return _bookmarkButton;
}

- (LocationImageView *)locationView
{
  if (!_locationView)
  {
    UIImage * image = [[UIImage imageNamed:@"PlacePageLocationBackground"] resizableImageWithCapInsets:UIEdgeInsetsMake(10, 10, 10, 10)];
    _locationView = [[LocationImageView alloc] initWithImage:image];
    _locationView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  }
  return _locationView;
}

- (UIButton *)guideButton
{
  if (!_guideButton)
  {
    UIImage * image = [[UIImage imageNamed:@"PlacePageGuideButton"] resizableImageWithCapInsets:UIEdgeInsetsMake(6, 6, 6, 6)];
    _guideButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, image.size.width, image.size.height)];
    _guideButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Bold" size:15];
    _guideButton.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    [_guideButton setBackgroundImage:image forState:UIControlStateNormal];
    [_guideButton addTarget:self action:@selector(guideButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
    [_guideButton setTitleColor:[UIColor colorWithColorCode:@"33cc66"] forState:UIControlStateNormal];

    UIView * contentView = [[UIView alloc] initWithFrame:_guideButton.bounds];
    contentView.userInteractionEnabled = NO;
    contentView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
    [_guideButton addSubview:contentView];

    UIImageView * guideImageView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 17, 17)];
    guideImageView.origin = CGPointMake(104, 10);
    [contentView addSubview:guideImageView];
    self.guideImageView = guideImageView;

    UILabel * leftLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 100, image.size.height)];
    leftLabel.backgroundColor = [UIColor clearColor];
    leftLabel.textAlignment = NSTextAlignmentRight;
    leftLabel.font = [UIFont fontWithName:@"HelveticaNeue-Bold" size:15];
    leftLabel.textColor = [UIColor colorWithColorCode:@"33cc66"];
    [contentView addSubview:leftLabel];
    self.guideLeftLabel = leftLabel;

    UILabel * rightLabel = [[UILabel alloc] initWithFrame:CGRectMake(125, 0, 150, image.size.height)];
    rightLabel.backgroundColor = [UIColor clearColor];
    rightLabel.font = [UIFont fontWithName:@"HelveticaNeue-Bold" size:15];
    rightLabel.textColor = [UIColor colorWithColorCode:@"33cc66"];
    [contentView addSubview:rightLabel];
    self.guideRightLabel = rightLabel;
  }
  return _guideButton;
}

- (UIButton *)editButton
{
  if (!_editButton)
  {
    UIImage * image = [[UIImage imageNamed:@"PlacePageButton"] resizableImageWithCapInsets:UIEdgeInsetsMake(6, 6, 6, 6)];
    _editButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, image.size.width, image.size.height)];
    _editButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:15];
    _editButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleWidth;
    [_editButton setBackgroundImage:image forState:UIControlStateNormal];
    [_editButton addTarget:self action:@selector(editButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
    [_editButton setTitle:NSLocalizedString(@"edit", nil) forState:UIControlStateNormal];
    [_editButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
  }
  return _editButton;
}

- (UIButton *)largeShareButton
{
  if (!_largeShareButton)
  {
    UIImage * image = [[UIImage imageNamed:@"PlacePageButton"] resizableImageWithCapInsets:UIEdgeInsetsMake(6, 6, 6, 6)];
    _largeShareButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, image.size.width, image.size.height)];
    _largeShareButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:15];
    _largeShareButton.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    [_largeShareButton setBackgroundImage:image forState:UIControlStateNormal];
    [_largeShareButton addTarget:self action:@selector(shareButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
    [_largeShareButton setTitle:NSLocalizedString(@"share", nil) forState:UIControlStateNormal];
    [_largeShareButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
  }
  return _largeShareButton;
}

- (UIButton *)shareButton
{
  if (!_shareButton)
  {
    UIImage * image = [[UIImage imageNamed:@"PlacePageButton"] resizableImageWithCapInsets:UIEdgeInsetsMake(6, 6, 6, 6)];
    _shareButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, image.size.width, image.size.height)];
    _shareButton.titleLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:15];
    _shareButton.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleWidth;
    [_shareButton setBackgroundImage:image forState:UIControlStateNormal];
    [_shareButton addTarget:self action:@selector(shareButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
    [_shareButton setTitle:NSLocalizedString(@"share", nil) forState:UIControlStateNormal];
    [_shareButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
  }
  return _shareButton;
}

- (UIScrollView *)scrollView
{
  if (!_scrollView)
  {
    _scrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 0, self.width, 0)];
    _scrollView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleBottomMargin;
  }
  return _scrollView;
}

- (m2::PointD)pinPoint
{
  return m_mark->GetOrg();
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [[MapsAppDelegate theApp].m_locationManager stop:self];
}

@end
