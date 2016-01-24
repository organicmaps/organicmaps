#import "MWMPlacePageTypeDescription.h"

namespace
{

NSString * const kPlacePageDescriptionViewNibName = @"MWMPlacePageDescriptionView";
CGFloat const kLeftOffset = 8.0;

} // namespace

@implementation MWMPlacePageTypeDescriptionView

- (void)layoutNearPoint:(CGPoint const &)point
{
  self.origin = {point.x + kLeftOffset, point.y};
}

@end

@interface MWMPlacePageELEDescription : MWMPlacePageTypeDescriptionView

@property (weak, nonatomic) IBOutlet UILabel * heightLabel;

- (void)configureWithHeight:(NSUInteger)height;

@end

@interface MWMPlacePageHotelDescription : MWMPlacePageTypeDescriptionView

- (void)configureWithStarsCount:(NSUInteger)count;

@end

@implementation MWMPlacePageTypeDescription

- (instancetype)initWithPlacePageEntity:(MWMPlacePageEntity *)entity
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:kPlacePageDescriptionViewNibName owner:self options:nil];
    if (entity.type == MWMPlacePageEntityTypeEle)
      [static_cast<MWMPlacePageELEDescription *>(self.eleDescription) configureWithHeight:entity.typeDescriptionValue];
    else
      [static_cast<MWMPlacePageHotelDescription *>(self.hotelDescription) configureWithStarsCount:entity.typeDescriptionValue];
    self.eleDescription.autoresizingMask = self.hotelDescription.autoresizingMask = UIViewAutoresizingNone;
  }
  return self;
}

@end

@implementation MWMPlacePageHotelDescription

- (void)configureWithStarsCount:(NSUInteger)count
{
  [self.subviews enumerateObjectsUsingBlock:^(UIImageView * star, NSUInteger idx, BOOL *stop)
  {
    star.highlighted = (idx < count);
  }];
}

@end

@implementation MWMPlacePageELEDescription

- (void)configureWithHeight:(NSUInteger)height
{
  self.heightLabel.text = [NSString stringWithFormat:@"%@", @(height)];
}

@end

