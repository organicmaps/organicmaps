
#import "SideToolbar.h"
#import "SideToolbarCell.h"
#import "UIKitCategories.h"
#import "BuyButtonCell.h"
#include "../../platform/platform.hpp"

typedef NS_ENUM(NSUInteger, Section)
{
  SectionTopSpace,
  SectionMenu,
  SectionBottomSpace,
  SectionBuyButton,
  SectionCount
};

@interface SideToolbar () <UITableViewDataSource, UITableViewDelegate, BuyButtonCellDelegate>

@property (nonatomic) UITableView * tableView;
@property (nonatomic) UIPanGestureRecognizer * slidePanGesture;
@property (nonatomic) UIPanGestureRecognizer * menuPanGesture;
@property (nonatomic) UIButton * buyButton;
@property (nonatomic) UIView * backgroundView;

@property (nonatomic) BOOL isMenuHidden;

@end

@implementation SideToolbar
{
  NSArray * menuTitles;
  NSArray * menuImageNames;
  NSArray * disabledInTrial;
  CGPoint slideViewOffset;
}

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  menuTitles = @[NSLocalizedString(@"search", nil),
                 NSLocalizedString(@"maps", nil),
                 NSLocalizedString(@"bookmarks", nil),
                 NSLocalizedString(@"settings", nil),
                 NSLocalizedString(@"share_my_location", nil)];

  menuImageNames = @[@"side-toolbar-icon-search",
                     @"side-toolbar-icon-map",
                     @"side-toolbar-icon-bookmarks",
                     @"side-toolbar-icon-settings",
                     @"side-toolbar-icon-share"];

  disabledInTrial = @[@YES, @NO, @YES, @NO, @NO, @NO];

  self.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleHeight;
  [self addGestureRecognizer:self.menuPanGesture];

  [self addSubview:self.backgroundView];
  [self addSubview:self.tableView];

  return self;
}

- (void)buyButtonCellDidPressBuyButton:(BuyButtonCell *)cell
{
  [self.delegate sideToolbarDidPressBuyButton:self];
}

#pragma mark - TableView

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (section == SectionMenu)
    return [menuTitles count];
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
      customCell.disabled = [disabledInTrial[indexPath.row] boolValue];

    customCell.iconImageView.image = [UIImage imageNamed:menuImageNames[indexPath.row]];
    customCell.titleLabel.text = menuTitles[indexPath.row];

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
  else if (indexPath.section == SectionBuyButton)
  {
    BuyButtonCell * customCell = (BuyButtonCell *)[tableView dequeueReusableCellWithIdentifier:@"BuyButtonCell"];
    if (!customCell)
    {
      customCell = [[BuyButtonCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"BuyButtonCell"];
      customCell.delegate = self;
    }
    cell = customCell;
  }

  return cell;
}

- (CGFloat)restSpace
{
  return self.height - [SideToolbarCell cellHeight] * [menuTitles count] - [BuyButtonCell cellHeight];
}

- (CGFloat)topSpaceRatio
{
  return IPAD ? 0.7 : 0.65;
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
  else if (indexPath.section == SectionBuyButton)
    return [BuyButtonCell cellHeight];
  return 0;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (indexPath.section == SectionMenu)
  {
    [self.delegate sideToolbar:self didPressButtonAtIndex:indexPath.row];
    [self.tableView deselectRowAtIndexPath:self.tableView.indexPathForSelectedRow animated:YES];
  }
}

- (void)layoutSubviews
{
  NSTimeInterval rotationDuration = 0.3;

  if (self.isMenuHidden)
    self.hidden = YES;
  [UIView animateWithDuration:rotationDuration delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    self.menuShift = self.isMenuHidden ? self.minimumMenuShift : self.maximumMenuShift;
  } completion:^(BOOL finished) {
    self.hidden = NO;
  }];
  CGFloat contentHeight = [self topSpaceHeight] + [SideToolbarCell cellHeight] * [menuTitles count] + [self bottomSpaceHeight] + [BuyButtonCell cellHeight];
  self.tableView.scrollEnabled = contentHeight > self.tableView.height;
  NSMutableIndexSet * indexSet = [[NSMutableIndexSet alloc] init];
  [indexSet addIndex:SectionTopSpace];
  [indexSet addIndex:SectionBottomSpace];
  [self.tableView reloadSections:indexSet withRowAnimation:UITableViewRowAnimationAutomatic];

  for (CALayer * backgroundLayer in self.backgroundView.layer.sublayers)
  {
    NSTimeInterval delay = backgroundLayer.frame.size.height > self.height ? rotationDuration : 0;
    [self performAfterDelay:delay block:^{
      CGRect rect = backgroundLayer.frame;
      rect.size.height = self.height;
      backgroundLayer.frame = rect;
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
  self.menuShift -= [sender translationInView:self.superview].x;
  [sender setTranslation:CGPointZero inView:self.superview];

  if (sender.state == UIGestureRecognizerStateBegan)
  {
    self.isMenuHidden = NO;
  }
  else if (sender.state == UIGestureRecognizerStateCancelled || sender.state == UIGestureRecognizerStateEnded)
  {
    CGFloat velocity = [sender velocityInView:self.superview].x;
    CGFloat minV = 1000;
    CGFloat maxV = 5000;
    BOOL isPositive = velocity > 0;
    CGFloat absV = ABS(velocity);
    absV = MIN(maxV, MAX(minV, absV));

    CGFloat magnetPercent = 0.25;
    CGFloat percent = self.menuShift / self.maximumMenuShift;

    if (percent < magnetPercent && absV < 2 * minV)
      [self setMenuHidden:YES animated:YES];
    else if (percent > 1 - magnetPercent && absV < 2 * minV)
      [self setMenuHidden:NO animated:YES];
    else
      [self setMenuHidden:isPositive duration:(300 / absV)];
  }
  [self.delegate sideToolbarDidUpdateShift:self];
}

#pragma mark - Setters

- (void)setMenuHidden:(BOOL)hidden duration:(NSTimeInterval)duration
{
  [UIView animateWithDuration:duration delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    self.menuShift = hidden ? self.minimumMenuShift : self.maximumMenuShift;
  } completion:^(BOOL finished) {}];
  self.isMenuHidden = hidden;
}

- (void)setMenuHidden:(BOOL)hidden animated:(BOOL)animated
{
  NSTimeInterval duration = animated ? 0.25 : 0;
  [self setMenuHidden:hidden duration:duration];
}

- (void)setMenuShift:(CGFloat)menuShift
{
  menuShift = MIN(self.maximumMenuShift, MAX(self.minimumMenuShift, menuShift));
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

- (void)setIsMenuHidden:(BOOL)isMenuHidden
{
  if (_isMenuHidden != isMenuHidden)
  {
    _isMenuHidden = isMenuHidden;
    [self didChangeValueForKey:@"isMenuHidden"];
  }
}

#pragma mark - Getters

- (CGFloat)minimumMenuShift
{
  return 0;
}

- (CGFloat)maximumMenuShift
{
  return self.width;
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
