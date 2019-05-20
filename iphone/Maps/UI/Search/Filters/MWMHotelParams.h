#import "MWMPlacePageData.h"

#include "map/place_page_info.hpp"
#include "map/booking_filter_params.hpp"

#include "search/hotels_filter.hpp"

#include "indexer/ftypes_matcher.hpp"

#include <memory>

enum class Price {
  Any,
  One,
  Two,
  Three
};

@interface MWMHotelParams : NSObject 

@property (nonatomic) std::unordered_set<Price> price;
@property (nonatomic) std::unordered_set<ftypes::IsHotelChecker::Type> types;
@property (nonatomic) place_page::rating::FilterRating rating;
@property (nonatomic) NSDate * checkInDate;
@property (nonatomic) NSDate * checkOutDate;

- (instancetype)initWithPlacePageData:(MWMPlacePageData *)data;

- (std::shared_ptr<search::hotels_filter::Rule>)rules;
- (booking::filter::Params)availabilityParams;

@end
