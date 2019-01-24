#import "MWMHotelParams.h"

static uint8_t kAdultsCount = 2;
static int8_t kAgeOfChild = 5;

@implementation MWMHotelParams

- (instancetype)initWithPlacePageData:(MWMPlacePageData *)data
{
  self = [super init];
  if (self)
  {
    _types.insert(ftypes::IsHotelChecker::Type::Hotel);
    CHECK(data.hotelType, ("Incorrect hotel type at coordinate:", data.latLon.lat, data.latLon.lon));
    
    if (data.isBooking)
    {
      if (auto const price = data.hotelRawApproximatePricing)
      {
        CHECK_LESS_OR_EQUAL(*price, base::Key(Price::Three), ());
        _price = static_cast<Price>(*price);
      }
      
      self.rating = place_page::rating::GetFilterRating(data.ratingRawValue);
    }
  }
  
  return self;
}

- (shared_ptr<search::hotels_filter::Rule>)rules
{
  using namespace search::hotels_filter;
  using namespace place_page::rating;
  
  shared_ptr<Rule> ratingRule;
  switch (self.rating)
  {
    case FilterRating::Any: break;
    case FilterRating::Good: ratingRule = Ge<Rating>(7.0); break;
    case FilterRating::VeryGood: ratingRule = Ge<Rating>(8.0); break;
    case FilterRating::Excellent: ratingRule = Ge<Rating>(9.0); break;
  }

  shared_ptr<Rule> priceRule;
  if (self.price != Any)
    priceRule = Or(priceRule, Eq<PriceRate>(self.price));
  
  shared_ptr<Rule> typeRule;
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
  return { make_shared<booking::AvailabilityParams>(params), {} };
}

@end
