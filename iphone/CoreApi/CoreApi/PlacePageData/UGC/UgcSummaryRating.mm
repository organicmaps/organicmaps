#import "UgcSummaryRating.h"

#include <CoreApi/Framework.h>

using namespace place_page;

static UgcSummaryRatingType convertRatingType(rating::Impress impress) {
  switch (impress) {
    case rating::None:
      return UgcSummaryRatingTypeNone;
    case rating::Horrible:
      return UgcSummaryRatingTypeHorrible;
    case rating::Bad:
      return UgcSummaryRatingTypeBad;
    case rating::Normal:
      return UgcSummaryRatingTypeNormal;
    case rating::Good:
      return UgcSummaryRatingTypeGood;
    case rating::Excellent:
      return UgcSummaryRatingTypeExcellent;
  }
}

@implementation UgcSummaryRating

- (instancetype)initWithRating:(float)ratingValue {
  self = [super init];
  if (self) {
    _ratingString = @(rating::GetRatingFormatted(ratingValue).c_str());
    _ratingType = convertRatingType(rating::GetImpress(ratingValue));
  }
  return self;
}

@end

