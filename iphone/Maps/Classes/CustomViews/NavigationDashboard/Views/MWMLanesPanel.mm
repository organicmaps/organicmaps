//
//  MWMLanesPanel.m
//  Maps
//
//  Created by v.mikhaylenko on 20.08.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMLanesPanel.h"
#import "UIKitCategories.h"
#import "UIColor+MapsMeColor.h"

static CGFloat const kOffset = 16.;
static CGFloat const kLaneSide = 28.;
static CGFloat const kBetweenOffset = 2.;

@interface MWMContentView : UIView

@end

@implementation MWMContentView

- (void)setCenter:(CGPoint)center
{
  super.center = center;
}

- (void)setFrame:(CGRect)frame
{
  super.frame = frame;
}

@end

@interface MWMLanesPanel ()

@property (nonatomic) UIView * contentView;
@property (nonatomic) NSUInteger numberOfLanes;

@end

@implementation MWMLanesPanel

- (instancetype)initWithParentView:(UIView *)parentView
{
  self = [super initWithFrame:{{}, {360., self.defaultHeight}}];
  if (self)
  {
    self.parentView = parentView;
    [self configure];
  }
  return self;
}

- (void)configure
{
  UIColor * background = UIColor.blackSecondaryText;
  self.contentView = [[MWMContentView alloc] initWithFrame:{{}, {0., self.defaultHeight}}];
  [self addSubview:self.contentView];
  if (IPAD)
  {
    self.backgroundColor = background;
    self.contentView.backgroundColor = UIColor.clearColor;
//    UIView * divider = [[UIView alloc] initWithFrame:{{}, {self.width, 1.}}];
  }
  else
  {
    self.backgroundColor = UIColor.clearColor;
    self.contentView.backgroundColor = background;
    self.contentView.layer.cornerRadius = 20.;
  }
  self.center = {self.parentView.center.x, self.center.y};
  self.hidden = YES;
  [self configureWithLanes:{}];
  self.autoresizingMask = UIViewAutoresizingFlexibleHeight;
  [self.parentView addSubview:self];
}

- (void)configureWithLanes:(std::vector<location::FollowingInfo::SingleLaneInfoClient> const &)lanes
{
  NSUInteger const size = lanes.size();
  if (size == self.numberOfLanes)
    return;
  self.numberOfLanes = size;
  if (size > 0)
  {
    self.contentView.width = size * kLaneSide + (size - 1) * kBetweenOffset + 2. * kOffset;
    self.contentView.center = {self.center.x, self.defaultHeight / 2};
    vector<UIImageView *> images;
    for (auto client : lanes)
      images.push_back([[UIImageView alloc] initWithImage:imageFromLane(client)]);

    CGFloat left = 16.;
    CGFloat const topLanes = 7.;
    CGFloat const topDividers = 32.;
    for (auto it = images.begin(); it != images.end(); ++it)
    {
      UIImageView * i = *it;
      [self.contentView addSubview:i];
      i.origin = {left, topLanes};
      left += kLaneSide + kBetweenOffset;
      if ([i isEqual:images.back()])
        break;

      UIView * d = divider();
      [self.contentView addSubview:d];
      d.origin = {i.maxX, topDividers};
    }
    [self layoutIfNeeded];
    self.hidden = NO;
  }
  else
  {
    self.hidden = YES;
  }
}

- (void)setHidden:(BOOL)hidden
{
  [super setHidden:hidden];
  if (hidden)
    [self.contentView.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];
}

UIImage * imageFromLane(location::FollowingInfo::SingleLaneInfoClient const & sl)
{
  auto ways = sl.m_lane;
  size_t const size = ways.size();
  if (size > 1)
  {
    vector<UIImage *> images;
    for (auto l : ways)
      images.push_back(image(static_cast<routing::turns::LaneWay>(l), sl.m_isRecommended));
    return image(images);
  }
  else
  {
    return image(static_cast<routing::turns::LaneWay>(sl.m_lane[0]), sl.m_isRecommended);
  }
}

UIImage * concatImages(UIImage * first, UIImage * second)
{
  CGSize const size = first.size;
  UIGraphicsBeginImageContextWithOptions(size, NO, 0.0);
  CGRect const frame {{0., 0.}, {size.width, size.height}};
  [first drawInRect:frame];
  [second drawInRect:frame];
  UIImage * newImage = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  return newImage;
}

UIImage * image(vector<UIImage *> & images)
{
  UIImage * result = images.back();
  images.pop_back();
  for (auto it = images.begin(); it != images.end(); ++it)
    result = concatImages(*it, result);
  return result;
}

UIImage * image(routing::turns::LaneWay lane, bool isRecommended)
{
  NSString * imageName;
  switch (lane)
  {
    case routing::turns::LaneWay::Through:
    case routing::turns::LaneWay::None:
      imageName = @"through_lines";
      break;
    case routing::turns::LaneWay::SlightRight:
    case routing::turns::LaneWay::Right:
    case routing::turns::LaneWay::SharpRight:
      imageName = @"right_add_lines";
      break;
    case routing::turns::LaneWay::SlightLeft:
    case routing::turns::LaneWay::Left:
    case routing::turns::LaneWay::SharpLeft:
      imageName = @"left_add_lines";
      break;
    case routing::turns::LaneWay::Reverse:
    case routing::turns::LaneWay::Count:
    case routing::turns::LaneWay::MergeToLeft:
    case routing::turns::LaneWay::MergeToRight:
      ASSERT(false, ("Incorrect lane instruction!"));
      imageName = @"through_lines";
      break;
  }
  return [UIImage imageNamed:[imageName stringByAppendingString:isRecommended ? @"_on" : @"_off"]];
}

UIView * divider()
{
  static CGRect const frame  {{0., 0.}, {2., 8.}};
  static UIColor * color = UIColor.whiteSecondaryText;
  UIView * result = [[UIView alloc] initWithFrame:frame];
  result.backgroundColor = color;
  return result;
}

@end
