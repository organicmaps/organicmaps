#import "MWMUGCViewModel.h"
#import "MWMPlacePageData.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "ugc/types.hpp"

#include "map/place_page_info.hpp"

using namespace place_page;

namespace
{
NSArray<MWMUGCRatingStars *> * starsRatings(ugc::Ratings const & ratings)
{
  NSMutableArray<MWMUGCRatingStars *> * mwmRatings = [@[] mutableCopy];
  for (auto const & rec : ratings)
    [mwmRatings addObject:[[MWMUGCRatingStars alloc] initWithTitle:@(rec.m_key.m_key.c_str())
                                                             value:rec.m_value
                                                          maxValue:5]];
  return [mwmRatings copy];
}

MWMUGCRatingValueType * ratingValueType(float rating)
{
  return [[MWMUGCRatingValueType alloc]
      initWithValue:@(rating::GetRatingFormatted(rating).c_str())
               type:[MWMPlacePageData ratingValueType:rating::GetImpress(rating)]];
}
}  // namespace

@interface MWMUGCViewModel ()
@property(copy, nonatomic) MWMVoidBlock refreshCallback;
@property(nonatomic) BOOL isFilled;
@end

@implementation MWMUGCViewModel
{
  place_page::Info m_info;
  ugc::UGC m_ugc;
  ugc::UGCUpdate m_ugcUpdate;
}

- (instancetype)initWithInfo:(place_page::Info const &)info refresh:(MWMVoidBlock)refresh
{
  self = [super init];
  if (self)
    m_info = info;

  if ([self isAvailable])
  {
    [self fill];
    self.refreshCallback = refresh;
  }

  return self;
}

- (BOOL)isAvailable { return m_info.ShouldShowUGC(); }
- (BOOL)canAddReview { return m_info.CanBeRated(); }
- (BOOL)canAddTextToReview { return m_info.CanBeRated() && m_info.CanBeReviewed(); }
- (BOOL)isYourReviewAvailable
{
  if (!self.isFilled)
    return NO;
  // TODO(ios, UGC): Add logic
  // return m_ugcUpdate.isValid();
  return YES;
}
- (BOOL)isReviewsAvailable { return [self numberOfReviews] != 0; }

- (void)fill
{
  self.isFilled = NO;
  auto & f = GetFramework();
  auto const featureID = m_info.GetID();
  f.GetUGCApi()->GetUGC(featureID, [self](ugc::UGC const & ugc, ugc::UGCUpdate const & update) {
    self->m_ugc = ugc;
    self->m_ugcUpdate = update;
    self.isFilled = YES;
    self.refreshCallback();
  });
}

- (NSInteger)ratingCellsCount { return 1; }
- (NSInteger)addReviewCellsCount { return [self isYourReviewAvailable] ? 0 : 1; }

- (NSInteger)totalReviewsCount
{
  // TODO(ios, UGC): Add logic
  // Used for display in string
  return 32;
}
- (MWMUGCRatingValueType *)summaryRating { return ratingValueType(m_ugc.m_aggRating); }

- (NSArray<MWMUGCRatingStars *> *)ratings
{
  NSAssert(self.isFilled, @"UGC is not filled");
  return starsRatings(m_ugc.m_ratings);
}

#pragma mark - MWMReviewsViewModelProtocol

- (NSInteger)numberOfReviews
{
  if (!self.isFilled)
    return 0;
  return m_ugc.m_reviews.size() + ([self isYourReviewAvailable] ? 1 : 0);
}

- (id<MWMReviewProtocol> _Nonnull)reviewWithIndex:(NSInteger)index
{
  NSAssert(self.isFilled, @"UGC is not filled");
  auto idx = index;
  NSAssert(idx >= 0, @"Invalid index");
  if ([self isYourReviewAvailable])
  {
    if (idx == 0)
    {
      auto const & review = m_ugcUpdate;
      return [[MWMUGCYourReview alloc]
          initWithDate:[NSDate
                           dateWithTimeIntervalSince1970:review.m_time.time_since_epoch().count()]
                  text:@(review.m_text.m_text.c_str())
               ratings:starsRatings(review.m_ratings)];
    }
    idx -= 1;
  }
  NSAssert(idx < m_ugc.m_reviews.size(), @"Invalid index");
  auto const & review = m_ugc.m_reviews[idx];
  return [[MWMUGCReview alloc]
      initWithTitle:@(review.m_author.m_name.c_str())
               date:[NSDate dateWithTimeIntervalSince1970:review.m_time.time_since_epoch().count()]
               text:@(review.m_text.m_text.c_str())
             rating:ratingValueType(review.m_rating)];
}

@end
