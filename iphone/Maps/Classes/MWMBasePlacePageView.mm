//
//  MWMBasePlagePageView.m
//  Maps
//
//  Created by v.mikhaylenko on 23.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMBasePlacePageView.h"
#import "MWMSpringAnimation.h"
#import "MWMPlacePageViewManager.h"
#import "UIKitCategories.h"
#import "MWMPlacePageInfoCell.h"
#import "MWMPlacePageActionBar.h"
#import "MWMPlacePageBookmarkCell.h"
#import "MWMPlacePageEntity.h"

#include "Framework.h"

static NSString * const kPlacePageLinkCellIdentifier = @"PlacePageLinkCell";
static NSString * const kPlacePageInfoCellIdentifier = @"PlacePageInfoCell";
static NSString * const kPlacePageBookmarkCellIdentifier = @"PlacePageBookmarkCell";
static NSString * const kPlacePageNibIdentifier = @"MWMBasePlacePage";
static CGFloat const kBookmarkCellHeight = 204.;

@interface MWMBasePlacePageView ()

@property (nonatomic) MWMSpringAnimation *springAnimation;
@property (nonatomic) MWMPlacePageEntity *entity;

@end

@interface MWMBasePlacePageView (UITableView) <UITableViewDelegate, UITableViewDataSource>
@end

@implementation MWMBasePlacePageView

- (instancetype)init
{
  self = [super init];
  if (self)
  {
    self = [[[NSBundle mainBundle] loadNibNamed:kPlacePageNibIdentifier owner:self options:nil] firstObject];
    [self.featureTable registerNib:[UINib nibWithNibName:kPlacePageInfoCellIdentifier bundle:nil] forCellReuseIdentifier:kPlacePageInfoCellIdentifier];
    [self.featureTable registerNib:[UINib nibWithNibName:kPlacePageLinkCellIdentifier bundle:nil] forCellReuseIdentifier:kPlacePageLinkCellIdentifier];
    [self.featureTable registerNib:[UINib nibWithNibName:kPlacePageBookmarkCellIdentifier bundle:nil] forCellReuseIdentifier:kPlacePageBookmarkCellIdentifier];
  }
  return self;
}

- (void)configureWithEntity:(MWMPlacePageEntity *)entity
{
  self.entity = entity;
  self.state = MWMPlacePageStateClosed;
  [self configure];
}

- (void)configure
{
  self.titleLabel.text = self.entity.title;
  self.typeLabel.text = self.entity.category;
  self.distanceLabel.text = self.entity.distance;
  [self.featureTable reloadData];

  CGFloat const titleOffset = 4.;
  CGFloat const typeOffset = 10.;
  [self.titleLabel sizeToFit];
  self.titleLabel.width = self.bounds.size.width * 0.7f;
  self.typeLabel.minY = self.titleLabel.maxY + titleOffset;
  self.featureTable.minY = self.typeLabel.maxY + typeOffset;
  self.directionArrow.center = CGPointMake(self.directionArrow.center.x, self.titleLabel.center.y);
  self.distanceLabel.center = CGPointMake(self.distanceLabel.center.x, self.titleLabel.center.y);
  self.featureTable.height = self.featureTable.contentSize.height;
}

- (void)show
{
  [self doesNotRecognizeSelector:_cmd];
}

- (CGPoint)targetPoint
{
  [self doesNotRecognizeSelector:_cmd];
  return CGPointZero;
}

- (IBAction)didTap:(UITapGestureRecognizer *)sender {
  [self doesNotRecognizeSelector:_cmd];
}

- (void)dismiss
{
  self.state = MWMPlacePageStateClosed;
  [self startAnimatingView:self initialVelocity:self.targetPoint];
  [UIView animateWithDuration:0.3f animations:^
  {
    self.actionBar.alpha = 0.;
  } completion:^(BOOL finished)
  {
    [self.actionBar removeFromSuperview];
    self.actionBar = nil;
    [self removeFromSuperview];
  }];
}

- (void)addBookmark
{
  NSUInteger count = [self.entity.metadata[@"keys"] count];
  [self.entity.metadata[@"keys"] insertObject:@"Bookmark" atIndex:count];
  [self.featureTable beginUpdates];
  [self.featureTable insertRowsAtIndexPaths:@[[NSIndexPath indexPathForRow:count inSection:0]] withRowAnimation:UITableViewRowAnimationAutomatic];
  [self.featureTable endUpdates];
  self.featureTable.height += kBookmarkCellHeight;

  //TODO (Vlad): Make setState: method.
  self.state = MWMPlacePageStatePreview;
  [self didTap:nil];
}

@end

@implementation MWMBasePlacePageView (Animations)

- (void)cancelSpringAnimation
{
  [self.placePageManager.ownerViewController.view.animator removeAnimation:self.springAnimation];
  self.springAnimation = nil;
}

- (void)startAnimatingView:(MWMBasePlacePageView *)view initialVelocity:(CGPoint)initialVelocity
{
  [self cancelSpringAnimation];
  self.springAnimation = [MWMSpringAnimation animationWithView:view target:view.targetPoint velocity:initialVelocity];
  [self.placePageManager.ownerViewController.view.animator addAnimation:self.springAnimation];
}

@end

@implementation MWMBasePlacePageView (UITableView)

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * const currentKey = self.entity.metadata[@"keys"][indexPath.row];

  if ([currentKey isEqualToString:@"Bookmark"])
    return kBookmarkCellHeight;

  static CGFloat const kDefaultCellHeight = 44.;
  UILabel *label = [[UILabel alloc] initWithFrame:CGRectMake(0., 0., self.bounds.size.width * 0.7f, 10)];
  label.numberOfLines = 0;
  label.text = self.entity.metadata[@"values"][indexPath.row];
  [label sizeToFit];
  CGFloat const defaultCellOffset = 22.;
  CGFloat const height = MAX(label.height + defaultCellOffset, kDefaultCellHeight);
  return height;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return [self.entity.metadata[@"keys"] count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * const currentKey = self.entity.metadata[@"keys"][indexPath.row];

  if ([currentKey isEqualToString:@"Bookmark"])
  {
    MWMPlacePageBookmarkCell *cell = (MWMPlacePageBookmarkCell *)[tableView dequeueReusableCellWithIdentifier:kPlacePageBookmarkCellIdentifier];

    if (cell == nil)
      cell = [[MWMPlacePageBookmarkCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:kPlacePageBookmarkCellIdentifier];

    cell.ownerTableView = tableView;
    [cell configure];
    return cell;
  }

  NSString * const kCellIdentifier = ([currentKey isEqualToString:@"PhoneNumber"] ||[currentKey isEqualToString:@"Email"] || [currentKey isEqualToString:@"Website"]) ? kPlacePageLinkCellIdentifier : kPlacePageInfoCellIdentifier;

  MWMPlacePageInfoCell *cell = (MWMPlacePageInfoCell *)[tableView dequeueReusableCellWithIdentifier:kCellIdentifier];

  if (cell == nil)
    cell = [[MWMPlacePageInfoCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:kCellIdentifier];

  [cell configureWithIconTitle:currentKey info:self.entity.metadata[@"values"][indexPath.row]];
  return cell;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event { }

@end
