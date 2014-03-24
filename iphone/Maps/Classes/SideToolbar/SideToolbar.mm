
#import "SideToolbar.h"
#import "SideToolbarCell.h"
#import "UIKitCategories.h"
#include "../../../../platform/platform.hpp"
#import "AppInfo.h"

typedef NS_ENUM(NSUInteger, Section)
{
  SectionTopSpace,
  SectionMenu,
  SectionBottomSpace,
  SectionCount
};

@interface SideToolbar () <UITableViewDataSource, UITableViewDelegate>

@property (nonatomic) UITableView * tableView;
@property (nonatomic) UIPanGestureRecognizer * slidePanGesture;
@property (nonatomic) UIPanGestureRecognizer * menuPanGesture;
@property (nonatomic) UIButton * buyButton;
@property (nonatomic) UIView * backgroundView;
@property (nonatomic) CGFloat startMenuShift;

@property (nonatomic) BOOL isMenuHidden;

@end

@implementation SideToolbar
{
  NSArray * menuItems;
  CGPoint slideViewOffset;
}

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  menuItems = @[@{@"Item" : @"Maps",      @"Title" : NSLocalizedString(@"download_maps", nil),     @"Icon" : @"IconMap",       @"Disabled" : @NO},
                @{@"Item" : @"Bookmarks", @"Title" : NSLocalizedString(@"bookmarks", nil),         @"Icon" : @"IconBookmarks", @"Disabled" : @YES},
                @{@"Item" : @"Settings",  @"Title" : NSLocalizedString(@"settings", nil),          @"Icon" : @"IconSettings",  @"Disabled" : @NO},
                @{@"Item" : @"Share",     @"Title" : NSLocalizedString(@"share_my_location", nil), @"Icon" : @"IconShare",     @"Disabled" : @NO},
                @{@"Item" : @"MoreApps",  @"Title" : NSLocalizedString(@"more_apps_title", nil),   @"Icon" : @"IconMoreApps",  @"Disabled" : @NO}];

  self.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleLeftMargin;
  [self addGestureRecognizer:self.menuPanGesture];

  [self addSubview:self.backgroundView];
  [self addSubview:self.tableView];

  return self;
}

#pragma mark - TableView

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (section == SectionMenu)
    return [menuItems count];
  else
    return 1;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return SectionCount;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  UITableViewCell * cell = nil;
  if (indexPath.section == SectionMenu)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:@"MenuCell"];
    SideToolbarCell * customCell = (SideToolbarCell *)cell;
    if (!customCell)
      customCell = [[SideToolbarCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"MenuCell"];

    if (GetPlatform().IsPro())
      customCell.disabled = NO;
    else
      customCell.disabled = [menuItems[indexPath.row][@"Disabled"] boolValue];

    customCell.iconImageView.image = [UIImage imageNamed:menuItems[indexPath.row][@"Icon"]];
    NSInteger wordsCount = [[menuItems[indexPath.row][@"Title"] componentsSeparatedByString:@" "] count];
    if (wordsCount > 1)
    {
      customCell.titleLabel.numberOfLines = 0;
      customCell.titleLabel.lineBreakMode = NSLineBreakByWordWrapping;
    }
    else
    {
      customCell.titleLabel.numberOfLines = 1;
      customCell.titleLabel.lineBreakMode = NSLineBreakByTruncatingTail;
    }
    customCell.titleLabel.text = menuItems[indexPath.row][@"Title"];

    cell = customCell;
  }
  else if (indexPath.section == SectionTopSpace || indexPath.section == SectionBottomSpace)
  {
    cell = [tableView dequeueReusableCellWithIdentifier:@"SpaceCell"];
    if (!cell)
    {
      cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"SpaceCell"];
      cell.backgroundColor = [UIColor clearColor];
      cell.selectionStyle = UITableViewCellSelectionStyleNone;
    }
  }

  return cell;
}

- (CGFloat)restSpace
{
  return self.height - [SideToolbarCell cellHeight] * [menuItems count];
}

- (CGFloat)topSpaceRatio
{
  return IPAD ? 0.7 : 0.5;
}

- (CGFloat)bottomSpaceRatio
{
  return 1 - [self topSpaceRatio];
}

- (CGFloat)topSpaceHeight
{
  return MAX(10, [self restSpace] * [self topSpaceRatio]);
}

- (CGFloat)bottomSpaceHeight
{
  return MAX(10, [self restSpace] * [self bottomSpaceRatio]);
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == SectionMenu)
    return [SideToolbarCell cellHeight];
  else if (indexPath.section == SectionTopSpace)
    return [self topSpaceHeight];
  else if (indexPath.section == SectionBottomSpace)
    return [self bottomSpaceHeight];
  return 0;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == SectionMenu)
  {
    [self.delegate sideToolbar:self didPressItemWithName:menuItems[indexPath.row][@"Item"]];
    [self.tableView deselectRowAtIndexPath:self.tableView.indexPathForSelectedRow animated:YES];
  }
}

- (void)layoutSubviews
{
  NSMutableIndexSet * indexSet = [[NSMutableIndexSet alloc] init];
  [indexSet addIndex:SectionTopSpace];
  [indexSet addIndex:SectionBottomSpace];
  [self.tableView reloadSections:indexSet withRowAnimation:UITableViewRowAnimationTop];

  for (CALayer * backgroundLayer in self.backgroundView.layer.sublayers)
  {
    NSTimeInterval delay = backgroundLayer.frame.size.height > self.height ? 0.3 : 0;
    [self performAfterDelay:delay block:^{
      CGRect frame = backgroundLayer.frame;
      frame.size.height = self.height;
      backgroundLayer.frame = frame;
    }];
  }
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {}

- (void)slidePanGesture:(UIPanGestureRecognizer *)sender
{
  [self panGesture:sender];
}

- (void)menuPanGesture:(UIPanGestureRecognizer *)sender
{
  [self panGesture:sender];
}

- (void)panGesture:(UIPanGestureRecognizer *)sender
{
  if (sender.state == UIGestureRecognizerStateBegan)
  {
    self.startMenuShift = self.menuShift;
    self.isMenuHidden = NO;
    [self.delegate sideToolbarWillOpenMenu:self];
  }

  self.menuShift = self.startMenuShift - [sender translationInView:self.superview].x;

  if (sender.state == UIGestureRecognizerStateCancelled || sender.state == UIGestureRecognizerStateEnded)
  {
    CGFloat velocity = [sender velocityInView:self.superview].x;
    CGFloat minV = 1000;
    CGFloat maxV = 5000;
    BOOL isPositive = velocity > 0;
    CGFloat absV = ABS(velocity);
    absV = MIN(maxV, MAX(minV, absV));
    double damping = isPositive ? 1 : 700 / absV;
    damping = MAX(damping, 0.55);
    [self setMenuHidden:isPositive duration:(400 / absV) damping:damping];
  }
  [self.delegate sideToolbarDidUpdateShift:self];
}

#pragma mark - Setters

- (void)setMenuHidden:(BOOL)hidden duration:(NSTimeInterval)duration damping:(double)damping
{
  self.isMenuHidden = hidden;
  if (hidden)
    [self.delegate sideToolbarWillCloseMenu:self];
  else
    [self.delegate sideToolbarWillOpenMenu:self];
  [UIView animateWithDuration:duration delay:0 damping:damping initialVelocity:0 options:(UIViewAnimationOptionCurveEaseInOut | UIViewAnimationOptionAllowUserInteraction) animations:^{
    self.menuShift = hidden ? self.minimumMenuShift : self.maximumMenuShift;
  } completion:^(BOOL finished){
    if (hidden)
      [self.delegate sideToolbarDidCloseMenu:self];
  }];
}

- (void)setMenuHidden:(BOOL)hidden animated:(BOOL)animated
{
  NSTimeInterval duration = animated ? 0.4 : 0;
  double damping = hidden ? 1 : 0.77;
  [self setMenuHidden:hidden duration:duration damping:damping];
}

- (void)setMenuShift:(CGFloat)menuShift
{
  CGFloat delta = menuShift - self.maximumMenuShift;
  if (delta > 0)
  {
    CGFloat springDistance = 40;
    CGFloat springStrength = 0.6;
    delta = MIN(springDistance, powf(delta, springStrength));
    menuShift = self.maximumMenuShift + delta;
  }

  menuShift = MAX(0, menuShift);
  self.minX = self.superview.width - menuShift;

  self.slideView.midX = self.minX - slideViewOffset.x;

  _menuShift = menuShift;
}

- (void)setSlideView:(UIView *)slideView
{
  slideViewOffset = CGPointMake(self.minX - slideView.center.x, self.minY - slideView.center.y);
  slideView.userInteractionEnabled = YES;
  [slideView addGestureRecognizer:self.slidePanGesture];
  _slideView = slideView;
}

#pragma mark - Getters

- (CGFloat)minimumMenuShift
{
  return 0;
}

- (CGFloat)maximumMenuShift
{
  return 260;
}

- (UIPanGestureRecognizer *)slidePanGesture
{
  if (!_slidePanGesture)
    _slidePanGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(slidePanGesture:)];

  return _slidePanGesture;
}

- (UIPanGestureRecognizer *)menuPanGesture
{
  if (!_menuPanGesture)
    _menuPanGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(menuPanGesture:)];

  return _menuPanGesture;
}

- (UITableView *)tableView
{
  if (!_tableView)
  {
    _tableView = [[UITableView alloc] initWithFrame:self.bounds];
    _tableView.autoresizingMask = UIViewAutoresizingFlexibleBottomMargin | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleHeight;
    _tableView.delegate = self;
    _tableView.dataSource = self;
    _tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
    _tableView.backgroundColor = [UIColor clearColor];
    _tableView.alwaysBounceVertical = NO;
  }
  return _tableView;
}

- (UIView *)backgroundView
{
  if (!_backgroundView)
  {
    _backgroundView = [[UIView alloc] initWithFrame:self.bounds];
    _backgroundView.autoresizingMask = UIViewAutoresizingFlexibleBottomMargin | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleHeight;

    CAGradientLayer * gradient = [CAGradientLayer layer];
    gradient.colors = @[(id)[[UIColor colorWithColorCode:@"2d2643"] CGColor], (id)[[UIColor colorWithColorCode:@"23355b"] CGColor]];
    gradient.startPoint = CGPointMake(0, 0);
    gradient.endPoint = CGPointMake(0, 1);
    gradient.locations = @[@0, @1];
    gradient.frame = _backgroundView.bounds;
    [_backgroundView.layer addSublayer:gradient];

    UIView * stripe = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 2.5, _backgroundView.height)];
    stripe.backgroundColor = [UIColor colorWithColorCode:@"000033"];
    [_backgroundView addSubview:stripe];
  }
  return _backgroundView;
}

@end
