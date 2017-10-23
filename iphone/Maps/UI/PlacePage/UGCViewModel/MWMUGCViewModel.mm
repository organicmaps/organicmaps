#import "MWMUGCViewModel.h"
#import "MWMPlacePageData.h"
#import "SwiftBridge.h"

#include "ugc/types.hpp"

#include <chrono>

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
@property(nonatomic) NSDateComponentsFormatter * formatter;
@end

@implementation MWMUGCViewModel
{
  place_page::Info m_info;
  ugc::UGC m_ugc;
  ugc::UGCUpdate m_ugcUpdate;
  std::vector<ugc::view_model::ReviewRow> m_reviewRows;
}

- (instancetype)initWithUGC:(ugc::UGC const &)ugc update:(ugc::UGCUpdate const &)update
{
  self = [super init];
  if (self)
  {
    m_ugc = ugc;
    m_ugcUpdate = update;
    [self fillReviewRows];
  }
  return self;
}

- (void)fillReviewRows
{
  using namespace ugc::view_model;
  m_reviewRows.clear();
  if (!m_ugcUpdate.IsEmpty())
    m_reviewRows.push_back(ReviewRow::YourReview);

  if (m_ugc.IsEmpty())
    return;

  auto const reviewsSize = m_ugc.m_reviews.size();
  auto constexpr kMaxSize = 3;
  if (reviewsSize > kMaxSize)
  {
    m_reviewRows.insert(m_reviewRows.end(), kMaxSize, ReviewRow::Review);
    m_reviewRows.push_back(ReviewRow::MoreReviews);
  }
  else
  {
    m_reviewRows.insert(m_reviewRows.end(), reviewsSize, ReviewRow::Review);
  }
}

- (BOOL)isUGCEmpty { return static_cast<BOOL>(m_ugc.IsEmpty()); }
- (BOOL)isUGCUpdateEmpty { return static_cast<BOOL>(m_ugcUpdate.IsEmpty()); }
- (NSUInteger)ratingCellsCount { return 1; }
- (NSUInteger)addReviewCellsCount { return 1; }
- (NSUInteger)totalReviewsCount { return static_cast<NSUInteger>(m_ugc.m_basedOn); }
- (MWMUGCRatingValueType *)summaryRating { return ratingValueType(m_ugc.m_totalRating); }
- (NSArray<MWMUGCRatingStars *> *)ratings { return starsRatings(m_ugc.m_ratings); }
- (std::vector<ugc::view_model::ReviewRow> const &)reviewRows { return m_reviewRows; }

#pragma mark - MWMReviewsViewModelProtocol

- (NSInteger)numberOfReviews { return m_ugc.m_reviews.size() + !self.isUGCUpdateEmpty; }

- (id<MWMReviewProtocol> _Nonnull)reviewWithIndex:(NSInteger)index
{
  auto idx = index;
  NSAssert(idx >= 0, @"Invalid index");
  if (!self.isUGCUpdateEmpty)
  {
    if (idx == 0)
    {
      auto const & review = m_ugcUpdate;
      return [[MWMUGCYourReview alloc]
              initWithDate:[self daysAgo:review.m_time]
                  text:@(review.m_text.m_text.c_str())
               ratings:starsRatings(review.m_ratings)];
    }
    idx -= 1;
  }
  NSAssert(idx < m_ugc.m_reviews.size(), @"Invalid index");
  auto const & review = m_ugc.m_reviews[idx];
  return [[MWMUGCReview alloc]
      initWithTitle:@(review.m_author.c_str())
               date:[self daysAgo:review.m_time]
               text:@(review.m_text.m_text.c_str())
             rating:ratingValueType(review.m_rating)];
}

#pragma mark - Propertis

- (NSString *)daysAgo:(ugc::Time const &) time
{
  using namespace std::chrono;
  NSDate * reviewDate = [NSDate dateWithTimeIntervalSince1970:duration_cast<seconds>(time.time_since_epoch()).count()];
  return [self.formatter stringFromDate:reviewDate toDate:[NSDate date]];
}

- (NSDateComponentsFormatter *)formatter
{
  if (!_formatter)
  {
    _formatter = [[NSDateComponentsFormatter alloc] init];
    _formatter.unitsStyle = NSDateComponentsFormatterUnitsStyleFull;
    _formatter.allowedUnits = NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay;
  }
  return _formatter;
}
@end
