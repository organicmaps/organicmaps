#import "MWMDiscoverySearchViewModel.h"
#import <CoreApi/UgcSummaryRatingType.h>

@interface MWMDiscoverySearchViewModel()

@property(nonatomic, readwrite) NSString *title;
@property(nonatomic, readwrite) NSString *subtitle;
@property(nonatomic, readwrite) NSString *distance;
@property(nonatomic, readwrite) BOOL isPopular;
@property(nonatomic, readwrite) NSString *ratingValue;
@property(nonatomic, readwrite) UgcSummaryRatingType ratingType;

@end

@implementation MWMDiscoverySearchViewModel

- (instancetype)initWithTitle:(NSString *)title
                     subtitle:(NSString *)subtitle
                     distance:(NSString *)distance
                    isPopular:(BOOL)isPopular
                  ratingValue:(NSString *) ratingValue
                   ratingType:(UgcSummaryRatingType)ratingType {
  self = [super init];
  if (self) {
    self.title = title;
    self.subtitle = subtitle;
    self.distance = distance;
    self.isPopular = isPopular;
    self.ratingValue = ratingValue;
    self.ratingType = ratingType;
  }
  return self;
}

@end
