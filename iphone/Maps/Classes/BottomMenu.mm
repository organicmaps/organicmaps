
#import "BottomMenu.h"
#import "BottomMenuCell.h"
#import "UIKitCategories.h"
#include "../../../platform/platform.hpp"
#import "AppInfo.h"

@interface BottomMenu () <UITableViewDataSource, UITableViewDelegate>

@property (nonatomic) UITableView * tableView;
@property (nonatomic) UIView * fadeView;

@property (nonatomic) NSArray * items;

@end

@implementation BottomMenu

- (instancetype)initWithFrame:(CGRect)frame items:(NSArray *)items
{
  self = [super initWithFrame:frame];

  self.items = items;

  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  [self addSubview:self.fadeView];

  CGFloat menuHeight = [items count] * [BottomMenuCell cellHeight];
  if (!GetPlatform().IsPro())
    menuHeight += [BottomMenuCell cellHeight];

  self.tableView.frame = CGRectMake(0, 0, self.width, menuHeight);
  [self addSubview:self.tableView];

  return self;
}

- (void)didMoveToSuperview
{
  [self setMenuHidden:YES animated:NO];
}

#pragma mark - TableView

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  if (GetPlatform().IsPro())
    return [self.items count];
  else
    return (section == 0) ? 1 : [self.items count];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  return GetPlatform().IsPro() ? 1 : 2;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (GetPlatform().IsPro() || (!GetPlatform().IsPro() && indexPath.section == 1))
  {
    BottomMenuCell * cell = (BottomMenuCell *)[tableView dequeueReusableCellWithIdentifier:[BottomMenuCell className]];
    if (!cell)
      cell = [[BottomMenuCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[BottomMenuCell className]];

    cell.iconImageView.image = self.items[indexPath.row][@"Icon"];
    NSInteger wordsCount = [[self.items[indexPath.row][@"Title"] componentsSeparatedByString:@" "] count];
    if (wordsCount > 1)
    {
      cell.titleLabel.numberOfLines = 0;
      cell.titleLabel.lineBreakMode = NSLineBreakByWordWrapping;
    }
    else
    {
      cell.titleLabel.numberOfLines = 1;
      cell.titleLabel.lineBreakMode = NSLineBreakByTruncatingTail;
    }
    cell.titleLabel.text = self.items[indexPath.row][@"Title"];

    return cell;
  }
  else
  {
    BottomMenuCell * cell = (BottomMenuCell *)[tableView dequeueReusableCellWithIdentifier:[BottomMenuCell className]];
    if (!cell)
      cell = [[BottomMenuCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:[BottomMenuCell className]];

    cell.titleLabel.text = NSLocalizedString(@"become_a_pro", nil);
    cell.iconImageView.image = [UIImage imageNamed:@"IconMWMPro"];

    return cell;
  }
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  return [BottomMenuCell cellHeight];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  if (GetPlatform().IsPro() || (!GetPlatform().IsPro() && indexPath.section == 1))
  {
    [self.delegate bottomMenu:self didPressItemWithName:self.items[indexPath.row][@"Item"]];
    [self.tableView deselectRowAtIndexPath:self.tableView.indexPathForSelectedRow animated:YES];
  }
  else
  {
    [self.delegate bottomMenuDidPressBuyButton:self];
  }
}

- (void)setMenuHidden:(BOOL)hidden animated:(BOOL)animated
{
  _menuHidden = hidden;
  [UIView animateWithDuration:(animated ? 0.25 : 0) delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    if (hidden)
    {
      self.userInteractionEnabled = NO;
      self.tableView.minY = self.tableView.superview.height;
      self.fadeView.alpha = 0;
    }
    else
    {
      self.userInteractionEnabled = YES;
      self.tableView.maxY = self.tableView.superview.height;
      self.fadeView.alpha = 1;
    }
  } completion:^(BOOL finished) {}];
}

- (void)tap:(UITapGestureRecognizer *)sender
{
  [self setMenuHidden:YES animated:YES];
}

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event
{
  return !self.menuHidden;
}

- (UITableView *)tableView
{
  if (!_tableView)
  {
    _tableView = [[UITableView alloc] initWithFrame:CGRectZero];
    _tableView.autoresizingMask = UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleWidth;
    _tableView.delegate = self;
    _tableView.dataSource = self;
    _tableView.scrollEnabled = NO;
    _tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
    _tableView.backgroundColor = [UIColor colorWithColorCode:@"444651"];
  }
  return _tableView;
}

- (UIView *)fadeView
{
  if (!_fadeView)
  {
    _fadeView = [[UIView alloc] initWithFrame:self.bounds];
    _fadeView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.5];
    _fadeView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
    UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap:)];
    [_fadeView addGestureRecognizer:tap];
  }
  return _fadeView;
}

@end
