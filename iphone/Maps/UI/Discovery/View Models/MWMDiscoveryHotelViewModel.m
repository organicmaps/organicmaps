#import "MWMDiscoveryHotelViewModel.h"

@interface MWMDiscoveryHotelViewModel()

@property(nonatomic, readwrite) NSString *title;
@property(nonatomic, readwrite) NSString *subtitle;
@property(nonatomic, readwrite) NSString *price;
@property(nonatomic, readwrite) NSString *distance;
@property(nonatomic, readwrite) BOOL isPopular;
@property(nonatomic, readwrite) NSString *ratingValue;
@property(nonatomic, readwrite) UgcSummaryRatingType ratingType;

@end

@implementation MWMDiscoveryHotelViewModel

- (instancetype)initWithTitle:(NSString *)title
                     subtitle:(NSString *)subtitle
                        price:(NSString *)price
                     distance:(NSString *)distance
                    isPopular:(BOOL)isPopular
                  ratingValue:(NSString *) ratingValue
                   ratingType:(UgcSummaryRatingType)ratingType {
  self = [super init];
  if (self) {
    self.title = title;
    self.subtitle = subtitle;
    self.price = price;
    self.distance = distance;
    self.isPopular = isPopular;
    self.ratingValue = ratingValue;
    self.ratingType = ratingType;
  }
  return self;
}

@end
