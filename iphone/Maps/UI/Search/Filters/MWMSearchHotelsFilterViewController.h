#import "MWMSearchFilterViewController.h"
#import "MWMTypes.h"

#include "map/place_page_info.hpp"

#include "indexer/ftypes_matcher.hpp"

namespace search_filter
{
enum class Price
{
  Any = 0,
  One = 1,
  Two = 2,
  Three = 3
};

struct HotelParams
{
  ftypes::IsHotelChecker::Type m_type = ftypes::IsHotelChecker::Type::Count;
  place_page::rating::FilterRating m_rating = place_page::rating::FilterRating::Any;
  Price m_price = Price::Any;
};
}  // namespace search_filter

@interface MWMSearchHotelsFilterViewController : MWMSearchFilterViewController

- (void)applyParams:(search_filter::HotelParams &&)params onFinishCallback:(MWMVoidBlock)callback;

@end
