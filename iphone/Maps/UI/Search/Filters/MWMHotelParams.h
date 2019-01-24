#import "MWMPlacePageData.h"

#include "indexer/ftypes_matcher.hpp"
#include "map/place_page_info.hpp"
#include "search/hotels_filter.hpp"
#include "map/booking_filter_params.hpp"

typedef enum {
  Any,
  One,
  Two,
  Three
} Price;

@interface MWMHotelParams : NSObject 

@property (nonatomic) Price price;
@property (nonatomic) std::unordered_set<ftypes::IsHotelChecker::Type> types;
@property (nonatomic) place_page::rating::FilterRating rating;
@property (nonatomic) NSDate * checkInDate;
@property (nonatomic) NSDate * checkOutDate;

- (instancetype)initWithPlacePageData:(MWMPlacePageData *)data;

- (shared_ptr<search::hotels_filter::Rule>)rules;
- (booking::filter::Params)availabilityParams;

@end
