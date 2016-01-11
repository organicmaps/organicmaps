#import "MWMPlacePageTypeDescription.h"

static NSString * const kPlacePageDescriptionViewNibName = @"MWMPlacePageDescriptionView";

@interface MWMPlacePageELEDescription : UIView

@property (weak, nonatomic) IBOutlet UILabel * heightLabel;

- (void)configureWithHeight:(NSUInteger)height;

@end

@interface MWMPlacePageHotelDescription : UIView

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
      [self.eleDescription configureWithHeight:entity.typeDescriptionValue];
    else
      [self.hotelDescription configureWithStarsCount:entity.typeDescriptionValue];
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

