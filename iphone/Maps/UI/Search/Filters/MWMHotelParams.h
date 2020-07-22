#include "map/booking_filter_params.hpp"
#include "map/place_page_info.hpp"

#include "search/hotels_filter.hpp"

#include "indexer/ftypes_matcher.hpp"

#include <memory>

enum class Price {
  Any,
  One,
  Two,
  Three
};

@class PlacePageData;

@interface MWMHotelParams : NSObject

@property(nonatomic) std::unordered_set<Price> price;
@property(nonatomic) std::unordered_set<ftypes::IsHotelChecker::Type> types;
@property(nonatomic) place_page::rating::FilterRating rating;
@property(nonatomic) NSDate *checkInDate;
@property(nonatomic) NSDate *checkOutDate;
@property(nonatomic) NSInteger numberOfRooms;
@property(nonatomic) NSInteger numberOfAdults;
@property(nonatomic) NSInteger numberOfChildren;
@property(nonatomic) NSInteger numberOfInfants;

- (instancetype)initWithPlacePageData:(PlacePageData *)data;
- (std::shared_ptr<search::hotels_filter::Rule>)rules;
- (int)rulesCount;
- (booking::filter::Params)availabilityParams;

@end
