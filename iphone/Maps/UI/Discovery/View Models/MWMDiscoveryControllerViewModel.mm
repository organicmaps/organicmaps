#import "MWMDiscoveryControllerViewModel.h"
#import "MWMDiscoveryCityGalleryObjects.h"
#import "MWMDiscoveryMapObjects.h"
#import "MWMDiscoveryHotelViewModel.h"
#import "MWMDiscoverySearchViewModel.h"

#import <CoreApi/CatalogPromoItem+Core.h>

#include "map/place_page_info.hpp"

#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"

#include "geometry/distance_on_sphere.hpp"

using namespace discovery;

@interface MWMDiscoveryControllerViewModel()

@property(nonatomic, readwrite) MWMDiscoveryMapObjects *attractions;
@property(nonatomic, readwrite) MWMDiscoveryMapObjects *cafes;
@property(nonatomic, readwrite) MWMDiscoveryMapObjects *hotels;
@property(nonatomic, readwrite) MWMDiscoveryCityGalleryObjects *guides;

@end

@implementation MWMDiscoveryControllerViewModel

- (void)updateMapObjects:(MWMDiscoveryMapObjects *)objects
                 forType:(ItemType const)type {
  switch (type) {
    case ItemType::Attractions:
      self.attractions = objects;
      break;
    case ItemType::Cafes:
      self.cafes = objects;
      break;
    case ItemType::Hotels:
      self.hotels = objects;
      break;
    case ItemType::Promo:
      break;
    case ItemType::LocalExperts:
      break;
  }
}

- (void)updateCityGalleryObjects:(MWMDiscoveryCityGalleryObjects *)objects {
  self.guides = objects;
}

- (NSUInteger)itemsCountForType:(ItemType const)type {
  switch (type) {
    case ItemType::Attractions:
      return self.attractions.count;
    case ItemType::Cafes:
      return self.cafes.count;
    case ItemType::Hotels:
      return self.hotels.count;
    case ItemType::Promo:
      return self.guides.count;
    case ItemType::LocalExperts:
      return 0;
  }
}

- (MWMDiscoverySearchViewModel *)attractionAtIndex:(NSUInteger)index {
  search::Result const &result = [self.attractions searchResultAtIndex:index];
  search::ProductInfo const &info = [self.attractions productInfoAtIndex:index];
  m2::PointD const &center = [self.attractions viewPortCenter];
  return [self searchViewModelForResult:result
                            productInfo:info
                         viewPortCenter:center];
}

- (MWMDiscoverySearchViewModel *)cafeAtIndex:(NSUInteger)index {
  search::Result const &result = [self.cafes searchResultAtIndex:index];
  search::ProductInfo const &info = [self.cafes productInfoAtIndex:index];
  m2::PointD const &center = [self.cafes viewPortCenter];
  return [self searchViewModelForResult:result
                            productInfo:info
                         viewPortCenter:center];
}

- (MWMDiscoveryHotelViewModel *)hotelAtIndex:(NSUInteger)index {
  search::Result const &result = [self.hotels searchResultAtIndex:index];
  search::ProductInfo const &info = [self.hotels productInfoAtIndex:index];
  m2::PointD const &center = [self.hotels viewPortCenter];
  return [self hotelViewModelForResult:result
                           productInfo:info
                        viewPortCenter:center];
}

- (CatalogPromoItem *)guideAtIndex:(NSUInteger)index {
  promo::CityGallery::Item const &item = [self.guides galleryItemAtIndex:index];
  return [self guideViewModelForItem:item];
}

#pragma mark - Builders

- (MWMDiscoverySearchViewModel *)searchViewModelForResult:(search::Result const &)result
                                              productInfo:(search::ProductInfo const &)info
                                           viewPortCenter:(m2::PointD const &)center {
  
  auto const readableType = classif().GetReadableObjectName(result.GetFeatureType());
  NSString *subtitle = @(platform::GetLocalizedTypeName(readableType).c_str());
  NSString *title = result.GetString().empty() ? subtitle : @(result.GetString().c_str());
  
  NSString *ratingValue = [self ratingValueForRating:info.m_ugcRating];
  UgcSummaryRatingType ratingType = [self ratingTypeForRating:info.m_ugcRating];
  
  NSString *distance = [self distanceFrom:center
                                       to:result.GetFeatureCenter()];
  
  BOOL isPopular = result.GetRankingInfo().m_popularity > 0;
  
  return [[MWMDiscoverySearchViewModel alloc] initWithTitle:title
                                                   subtitle:subtitle
                                                   distance:distance
                                                  isPopular:isPopular
                                                ratingValue:ratingValue
                                                 ratingType:ratingType];
}

- (MWMDiscoveryHotelViewModel *)hotelViewModelForResult:(search::Result const &)result
                                            productInfo:(search::ProductInfo const &)info
                                         viewPortCenter:(m2::PointD const &)center {
  
  auto const readableType = classif().GetReadableObjectName(result.GetFeatureType());
  NSString *subtitle = @(platform::GetLocalizedTypeName(readableType).c_str());
  NSString *title = result.GetString().empty() ? subtitle : @(result.GetString().c_str());
  NSUInteger starsCount = result.GetStarsCount();
  if (starsCount > 0) {
    subtitle = [@"" stringByPaddingToLength:starsCount
                                 withString:@"★"
                            startingAtIndex:0];
  }
  NSString *price = @(result.GetHotelApproximatePricing().c_str());
  
  NSString *ratingValue = [self ratingValueForRating:result.GetHotelRating()];
  UgcSummaryRatingType ratingType = [self ratingTypeForRating:result.GetHotelRating()];
  
  NSString *distance = [self distanceFrom:center
                                       to:result.GetFeatureCenter()];
  
  BOOL isPopular = result.GetRankingInfo().m_popularity > 0;
  
  return [[MWMDiscoveryHotelViewModel alloc] initWithTitle:title
                                                  subtitle:subtitle
                                                     price:price
                                                  distance:distance
                                                 isPopular:isPopular
                                               ratingValue:ratingValue
                                                ratingType:ratingType];
}

- (CatalogPromoItem *)guideViewModelForItem:(promo::CityGallery::Item const &)item {
  return [[CatalogPromoItem alloc] initWithCoreItem:item];
}

#pragma mark - Helpers

- (NSString *)distanceFrom:(m2::PointD const &)startPoint
                        to:(m2::PointD const &)endPoint {
  auto const localizedUnits = platform::GetLocalizedDistanceUnits();
  auto const f = mercator::ToLatLon(startPoint);
  auto const t = mercator::ToLatLon(endPoint);
  auto const distance = ms::DistanceOnEarth(f.m_lat, f.m_lon, t.m_lat, t.m_lon);
  return @(measurement_utils::FormatDistanceWithLocalization(distance, localizedUnits.m_high,
                                                             localizedUnits.m_low).c_str());
}

- (NSString *)ratingValueForRating:(float)rating {
  return @(place_page::rating::GetRatingFormatted(rating).c_str());
}

- (UgcSummaryRatingType)ratingTypeForRating:(float)rating {
  return (UgcSummaryRatingType)place_page::rating::GetImpress(rating);
}

@end
