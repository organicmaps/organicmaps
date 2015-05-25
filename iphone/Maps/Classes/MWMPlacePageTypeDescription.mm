//
//  MWMPlacePageTypeDescription.m
//  Maps
//
//  Created by v.mikhaylenko on 07.06.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMPlacePageTypeDescription.h"
#import "UIKitCategories.h"

@interface MWMPlacePageELEDescription : UIView

@property (weak, nonatomic) IBOutlet UILabel * heightLabel;

@end

@interface MWMPlacePageHotelDescription : UIView

- (void)configureWithStarsCount:(NSUInteger)count;

@end

static NSString * const kPlacePageDescriptionViewNibName = @"MWMPlacePageDescriptionView";

@implementation MWMPlacePageTypeDescription

- (instancetype)initWithPlacePageEntity:(MWMPlacePageEntity const *)entity
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:kPlacePageDescriptionViewNibName owner:self options:nil];
    if (entity.type == MWMPlacePageEntityTypeEle)
      self.eleDescription.heightLabel.text = [NSString stringWithFormat:@"%@", @(entity.typeDescriptionValue)];
    else
      [self.hotelDescription configureWithStarsCount:entity.typeDescriptionValue];
  }
  return self;
}

@end

@implementation MWMPlacePageELEDescription

@end

@implementation MWMPlacePageHotelDescription

- (void)configureWithStarsCount:(NSUInteger)count
{
  [self.subviews enumerateObjectsUsingBlock:^(UIImageView * star, NSUInteger idx, BOOL *stop)
  {
    if (idx < count)
      star.image = [UIImage imageNamed:@"hotel_star_on"];
    else
      star.image = [UIImage imageNamed:@"hotel_star_off"];
  }];
}

@end
