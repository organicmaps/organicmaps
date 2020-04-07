#import "MWMHotelParams.h"

#include <CoreApi/Framework.h>
#include <CoreApi/PlacePageData.h>
#include <CoreApi/PlacePagePreviewData.h>
#include <CoreApi/HotelBookingData.h>

static uint8_t kAdultsCount = 2;
static int8_t kAgeOfChild = 5;

@implementation MWMHotelParams

- (instancetype)initWithPlacePageData:(PlacePageData *)data
{
  self = [super init];
  if (self)
  {
    _types.insert(ftypes::IsHotelChecker::Type::Hotel);

    PlacePagePreviewData *previewData = data.previewData;
    CHECK(previewData.hotelType != PlacePageDataHotelTypeNone,
          ("Incorrect hotel type at coordinate:", data.locationCoordinate.latitude, data.locationCoordinate.longitude));
    
    if (data.sponsoredType == PlacePageSponsoredTypeBooking)
    {
      if (auto const price = [previewData.rawPricing intValue])
      {
        CHECK_LESS_OR_EQUAL(price, base::Underlying(Price::Three), ());
        _price.insert(static_cast<Price>(price));
      }
      
      self.rating = place_page::rating::GetFilterRating(previewData.rawRating);
    }
  }
  
  return self;
}

- (std::shared_ptr<search::hotels_filter::Rule>)rules
{
  using namespace search::hotels_filter;
  using namespace place_page::rating;
  
  std::shared_ptr<Rule> ratingRule;
  switch (self.rating)
  {
    case FilterRating::Any: break;
    case FilterRating::Good: ratingRule = Ge<Rating>(7.0); break;
    case FilterRating::VeryGood: ratingRule = Ge<Rating>(8.0); break;
    case FilterRating::Excellent: ratingRule = Ge<Rating>(9.0); break;
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

unsigned makeMask(std::unordered_set<ftypes::IsHotelChecker::Type> const & items)
{
  unsigned mask = 0;
  for (auto const i : items)
    mask = mask | 1U << static_cast<unsigned>(i);
  
  return mask;
}

- (booking::filter::Params)availabilityParams
{
  using Clock = booking::AvailabilityParams::Clock;
  booking::AvailabilityParams params;
  params.m_rooms = {{kAdultsCount, kAgeOfChild}};
  if (Platform::IsConnected())
  {
    params.m_checkin = Clock::from_time_t(self.checkInDate.timeIntervalSince1970);
    params.m_checkout = Clock::from_time_t(self.checkOutDate.timeIntervalSince1970);
  }
  return { std::make_shared<booking::AvailabilityParams>(params), {} };
}

@end
