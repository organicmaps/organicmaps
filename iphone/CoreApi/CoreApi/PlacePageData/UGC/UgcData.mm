#import "UgcData+Core.h"

#import "UgcSummaryRating.h"

using namespace place_page;

@implementation UgcStarRating

- (instancetype)initWithRating:(ugc::RatingRecord const &)ratingRecord {
  self = [super init];
  if (self) {
    _title = @(ratingRecord.m_key.m_key.c_str());
    _value = ratingRecord.m_value;
  }
  return self;
}

@end

@implementation UgcReview

- (instancetype)initWithReview:(ugc::Review const &)review {
  self = [super init];
  if (self) {
    _reviewId = review.m_id;
    _author = @(review.m_author.c_str());
    _text = @(review.m_text.m_text.c_str());
    _rating = review.m_rating;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(review.m_time.time_since_epoch()).count();
    _date = [NSDate dateWithTimeIntervalSince1970:seconds];
  }
  return self;
}

@end

@implementation UgcMyReview

- (instancetype)initWithUgcUpdate:(ugc::UGCUpdate const &)ugcUpdate {
  self = [super init];
  if (self) {
    _text = @(ugcUpdate.m_text.m_text.c_str());
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(ugcUpdate.m_time.time_since_epoch()).count();
    _date = [NSDate dateWithTimeIntervalSince1970:seconds];

    NSMutableArray *ratingsArray = [NSMutableArray arrayWithCapacity:ugcUpdate.m_ratings.size()];
    for (auto const &r : ugcUpdate.m_ratings) {
      UgcStarRating *rating = [[UgcStarRating alloc] initWithRating:r];
      [ratingsArray addObject:rating];
    }
    _starRatings = [ratingsArray copy];
  }
  return self;
}

@end

@implementation UgcData

@end

@implementation UgcData (Core)

- initWithUgc:(ugc::UGC const &)ugc ugcUpdate:(ugc::UGCUpdate const &)update {
  self = [super init];
  if (self) {
    _isEmpty = ugc.IsEmpty();
    _isUpdateEmpty = update.IsEmpty();
    _isTotalRatingEmpty = ugc.m_totalRating == kIncorrectRating;
    _ratingsCount = ugc.m_basedOn;

    if (!_isTotalRatingEmpty) {
      _summaryRating = [[UgcSummaryRating alloc] initWithRating:ugc.m_totalRating];
    }

    if (!_isUpdateEmpty) {
      _myReview = [[UgcMyReview alloc] initWithUgcUpdate:update];
    }

    NSMutableArray *ratingsArray = [NSMutableArray arrayWithCapacity:ugc.m_ratings.size()];
    for (auto const &r : ugc.m_ratings) {
      UgcStarRating *rating = [[UgcStarRating alloc] initWithRating:r];
      [ratingsArray addObject:rating];
    }
    _starRatings = [ratingsArray copy];

    NSMutableArray *reviewsArray = [NSMutableArray arrayWithCapacity:ugc.m_reviews.size()];
    for (auto const &r : ugc.m_reviews) {
      UgcReview *review = [[UgcReview alloc] initWithReview:r];
      [reviewsArray addObject:review];
    }
    _reviews = [reviewsArray copy];
  }
  return self;
}

@end


