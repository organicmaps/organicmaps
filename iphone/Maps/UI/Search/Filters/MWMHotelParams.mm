#import "MWMHotelParams.h"

#include <CoreApi/Framework.h>
#include <CoreApi/HotelBookingData.h>
#include <CoreApi/PlacePageData.h>
#include <CoreApi/PlacePagePreviewData.h>

static int8_t kAgeOfChild = 8;
static int8_t kAgeOfInfant = 2;

@interface Room: NSObject

@property (nonatomic) NSInteger adults;
@property (nonatomic) NSInteger children;
@property (nonatomic) NSInteger infants;

@end

@implementation Room

@end

@implementation MWMHotelParams

- (instancetype)initWithPlacePageData:(PlacePageData *)data
{
  self = [self init];
  if (self)
  {
    _types.insert(ftypes::IsHotelChecker::Type::Hotel);

    PlacePagePreviewData *previewData = data.previewData;
    CHECK(previewData.hotelType != PlacePageDataHotelTypeNone,
          ("Incorrect hotel type at coordinate:", data.locationCoordinate.latitude, data.locationCoordinate.longitude));

    if (data.sponsoredType == PlacePageSponsoredTypeBooking) {
      if (auto const price = [previewData.rawPricing intValue]) {
        CHECK_LESS_OR_EQUAL(price, base::Underlying(Price::Three), ());
        _price.insert(static_cast<Price>(price));
      }

      self.rating = place_page::rating::GetFilterRating(previewData.rawRating);
    }
  }

  return self;
}

- (std::shared_ptr<search::hotels_filter::Rule>)rules {
  using namespace search::hotels_filter;
  using namespace place_page::rating;

  std::shared_ptr<Rule> ratingRule;
  switch (self.rating) {
    case FilterRating::Any:
      break;
    case FilterRating::Good:
      ratingRule = Ge<Rating>(7.0);
      break;
    case FilterRating::VeryGood:
      ratingRule = Ge<Rating>(8.0);
      break;
    case FilterRating::Excellent:
      ratingRule = Ge<Rating>(9.0);
      break;
  }

  std::shared_ptr<Rule> priceRule;
  for (auto const filter : self.price) {
    if (filter != Price::Any)
      priceRule = Or(priceRule, Eq<PriceRate>(static_cast<unsigned>(filter)));
  }

  std::shared_ptr<Rule> typeRule;
  if (!self.types.empty())
    typeRule = OneOf(makeMask(self.types));

  if (!ratingRule && !priceRule && !typeRule)
    return nullptr;

  return And(And(ratingRule, priceRule), typeRule);
}

- (int)rulesCount {
  int result = 0;
  using namespace search::hotels_filter;
  using namespace place_page::rating;

  if (self.rating != FilterRating::Any) {
    result += 1;
  }

  for (auto const filter : self.price) {
    if (filter != Price::Any) {
      result += 1;
      break;
    }
  }

  if (!self.types.empty()) {
    result += 1;
  }

  return result;
}

unsigned makeMask(std::unordered_set<ftypes::IsHotelChecker::Type> const &items) {
  unsigned mask = 0;
  for (auto const i : items)
    mask = mask | 1U << static_cast<unsigned>(i);

  return mask;
}

- (booking::filter::Params)availabilityParams {
  using Clock = booking::OrderingParams::Clock;
  booking::AvailabilityParams params;
  if (Platform::IsConnected())
  {
    NSInteger roomsCount = self.numberOfRooms > self.numberOfAdults ? self.numberOfAdults : self.numberOfRooms;
    NSMutableArray<Room *> *rooms = [NSMutableArray array];
    NSInteger i = 0;
    while (i < self.numberOfAdults + self.numberOfChildren + self.numberOfInfants) {
      Room *room;
      NSInteger roomIndex = i % roomsCount;
      if (i <= roomIndex) {
        room = [[Room alloc] init];
        [rooms addObject:room];
      } else {
        room = rooms[roomIndex];
      }
      if (i >= self.numberOfAdults + self.numberOfChildren) {
        room.infants += 1;
      } else if (i >= self.numberOfAdults) {
        room.children += 1;
      } else {
        room.adults += 1;
      }
      i += 1;
    }

    std::vector<booking::OrderingParams::Room> filterRooms;
    filterRooms.reserve(rooms.count);
    for (Room *room : rooms) {
      std::vector<int8_t> agesOfChildren(room.children, kAgeOfChild);
      agesOfChildren.insert(agesOfChildren.end(), room.infants, kAgeOfInfant);
      filterRooms.emplace_back(room.adults, agesOfChildren);
    }

    auto &orderingParams = params.m_orderingParams;
    orderingParams.m_rooms = filterRooms;
    NSInteger checkInOffset = [NSTimeZone.systemTimeZone secondsFromGMTForDate:self.checkInDate];
    NSInteger checkOutOffset = [NSTimeZone.systemTimeZone secondsFromGMTForDate:self.checkOutDate];
    orderingParams.m_checkin = Clock::from_time_t(self.checkInDate.timeIntervalSince1970 + checkInOffset);
    orderingParams.m_checkout = Clock::from_time_t(self.checkOutDate.timeIntervalSince1970 + checkOutOffset);
  }
  return {std::make_shared<booking::AvailabilityParams>(params), {}};
}

@end
