#import "HotelBookingData+Core.h"

@implementation HotelFacility

- (instancetype)initWithFacility:(booking::HotelFacility const &)facility {
  self = [super init];
  if (self) {
    _type = @(facility.m_type.c_str());
    _name = @(facility.m_name.c_str());
  }
  return self;
}

@end

@implementation HotelPhotoUrl

- (instancetype)initWithPhotoUrl:(booking::HotelPhotoUrls const &)photoUrl {
  self = [super init];
  if (self) {
    _original = @(photoUrl.m_original.c_str());
    _thumbnail = @(photoUrl.m_small.c_str());
  }
  return self;
}

@end

@implementation HotelReview

- (instancetype)initWithHotelReview:(booking::HotelReview const &)review {
  self = [super init];
  if (self) {
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(review.m_date.time_since_epoch()).count();
    _date = [NSDate dateWithTimeIntervalSince1970:seconds];
    _score = review.m_score;
    _author = @(review.m_author.c_str());
    _pros = review.m_pros.empty() ? nil : @(review.m_pros.c_str());
    _cons = review.m_cons.empty() ? nil : @(review.m_cons.c_str());
  }
  return self;
}

@end

@implementation HotelBookingData

@end

@implementation HotelBookingData (Core)

- (instancetype)initWithHotelInfo:(booking::HotelInfo const &)hotelInfo {
  self = [super init];
  if (self) {
    _hotelId = @(hotelInfo.m_hotelId.c_str());
    _hotelDescription = @(hotelInfo.m_description.c_str());
    _score = hotelInfo.m_score;
    _scoreCount = hotelInfo.m_scoreCount;
    NSMutableArray *facilitiesArray = [NSMutableArray arrayWithCapacity:hotelInfo.m_facilities.size()];
    for (auto const &f : hotelInfo.m_facilities) {
      HotelFacility *facility = [[HotelFacility alloc] initWithFacility:f];
      [facilitiesArray addObject:facility];
    }
    _facilities = [facilitiesArray copy];

    NSMutableArray *photosArray = [NSMutableArray arrayWithCapacity:hotelInfo.m_photos.size()];
    for (auto const &ph : hotelInfo.m_photos) {
      HotelPhotoUrl *photo = [[HotelPhotoUrl alloc] initWithPhotoUrl:ph];
      [photosArray addObject:photo];
    }
    _photos = [photosArray copy];

    NSMutableArray *reviewsArray = [NSMutableArray arrayWithCapacity:hotelInfo.m_reviews.size()];
    for (auto const &r : hotelInfo.m_reviews) {
      HotelReview *review = [[HotelReview alloc] initWithHotelReview:r];
      [reviewsArray addObject:review];
    }
    _reviews = [reviewsArray copy];
  }
  return self;
}

@end
