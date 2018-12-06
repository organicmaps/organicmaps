#import "MWMSearchCommonCell.h"
#import "CLLocation+Mercator.h"
#import "MWMLocationManager.h"

#include "map/place_page_info.hpp"

#include "search/result.hpp"

#include "indexer/classificator.hpp"

#include "geometry/mercator.hpp"

#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"

#include "defines.hpp"

namespace
{
bool PopularityHasHigherPriority(bool hasPosition, double distanceInMeters)
{
  return !hasPosition || distanceInMeters > search::Result::kPopularityHighPriorityMinDistance;
}
}  // namespace

@interface MWMSearchCommonCell ()

@property(nonatomic) IBOutletCollection(UIImageView) NSArray * infoRatingStars;
@property(weak, nonatomic) IBOutlet UILabel * distanceLabel;
@property(weak, nonatomic) IBOutlet UILabel * infoLabel;
@property(weak, nonatomic) IBOutlet UILabel * locationLabel;
@property(weak, nonatomic) IBOutlet UILabel * priceLabel;
@property(weak, nonatomic) IBOutlet UILabel * ratingLabel;
@property(weak, nonatomic) IBOutlet UILabel * typeLabel;
@property(weak, nonatomic) IBOutlet UIView * closedView;
@property(weak, nonatomic) IBOutlet UIView * infoRatingView;
@property(weak, nonatomic) IBOutlet UIView * infoView;
@property(weak, nonatomic) IBOutlet UIView * availableView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * availableTypeOffset;
@property(weak, nonatomic) IBOutlet UIView * sideAvailableMarker;
@property(weak, nonatomic) IBOutlet UIImageView * hotOfferImageView;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * priceOffset;
@property(weak, nonatomic) IBOutlet UIView * popularView;

@end

@implementation MWMSearchCommonCell

- (void)config:(search::Result const &)result
    isAvailable:(BOOL)isAvailable
    isHotOffer:(BOOL)isHotOffer
    productInfo:(search::ProductInfo const &)productInfo
    localizedTypeName:(NSString *)localizedTypeName
{
  [super config:result localizedTypeName:localizedTypeName];

  self.typeLabel.text = localizedTypeName;

  auto const hotelRating = result.GetHotelRating();
  auto const ugcRating = productInfo.m_ugcRating;
  auto const rating = hotelRating != kInvalidRatingValue ? hotelRating : ugcRating;
  if (rating != kInvalidRatingValue)
  {
    auto const str = place_page::rating::GetRatingFormatted(rating);
    self.ratingLabel.text = [NSString stringWithFormat:L(@"place_page_booking_rating"), str.c_str()];
  }
  else
  {
    self.ratingLabel.text = @"";
  }

  self.priceLabel.text = @(result.GetHotelApproximatePricing().c_str());
  self.locationLabel.text = @(result.GetAddress().c_str());
  [self.locationLabel sizeToFit];

  self.availableTypeOffset.priority = UILayoutPriorityDefaultHigh;
  self.availableView.hidden = !isAvailable;
  self.sideAvailableMarker.hidden = !isAvailable;
  self.hotOfferImageView.hidden = !isHotOffer;
  self.priceOffset.priority = isHotOffer ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;

  NSUInteger const starsCount = result.GetStarsCount();
  NSString * cuisine = @(result.GetCuisine().c_str()).capitalizedString;
  NSString * airportIata = @(result.GetAirportIata().c_str());
  NSString * brand  = @"";
  if (!result.GetBrand().empty())
    brand = @(platform::GetLocalizedBrandName(result.GetBrand()).c_str());

  if (starsCount > 0)
    [self setInfoRating:starsCount];
  else if (airportIata.length > 0)
    [self setInfoText:airportIata];
  else if (brand.length > 0 && cuisine.length > 0)
    [self setInfoText:[NSString stringWithFormat:@"%@ • %@", brand, cuisine]];
  else if (brand.length > 0)
    [self setInfoText:brand];
  else if (cuisine.length > 0)
    [self setInfoText:cuisine];
  else
    [self clearInfo];

  CLLocation * lastLocation = [MWMLocationManager lastLocation];
  double distanceInMeters = 0.0;
  if (lastLocation)
  {
    if (result.HasPoint())
    {
      distanceInMeters =
          MercatorBounds::DistanceOnEarth(lastLocation.mercator, result.GetFeatureCenter());
      string distanceStr;
      measurement_utils::FormatDistance(distanceInMeters, distanceStr);

      self.distanceLabel.text = @(distanceStr.c_str());
    }
  }

  bool popularityHasHigherPriority = PopularityHasHigherPriority(lastLocation, distanceInMeters);
  bool showClosed = result.IsOpenNow() == osm::No;
  bool showPopular = result.GetRankingInfo().m_popularity > 0;

  if (showClosed && showPopular)
  {
    self.closedView.hidden = popularityHasHigherPriority;
    self.popularView.hidden = !popularityHasHigherPriority;
  }
  else
  {
    self.closedView.hidden = !showClosed;
    self.popularView.hidden = !showPopular;
  }

  if (productInfo.m_isLocalAdsCustomer)
    self.backgroundColor = [UIColor bannerBackground];
  else if (isAvailable)
    self.backgroundColor = [UIColor transparentGreen];
  else
    self.backgroundColor = [UIColor white];
}

- (void)setInfoText:(NSString *)infoText
{
  self.infoView.hidden = NO;
  self.infoLabel.hidden = NO;
  self.infoRatingView.hidden = YES;
  self.infoLabel.text = infoText;
  self.availableTypeOffset.priority = UILayoutPriorityDefaultLow;
}

- (void)setInfoRating:(NSUInteger)infoRating
{
  self.infoView.hidden = NO;
  self.infoRatingView.hidden = NO;
  self.infoLabel.hidden = YES;
  self.availableTypeOffset.priority = UILayoutPriorityDefaultLow;
  [self.infoRatingStars
      enumerateObjectsUsingBlock:^(UIImageView * star, NSUInteger idx, BOOL * stop) {
        star.highlighted = star.tag <= infoRating;
      }];
}

- (void)clearInfo { self.infoView.hidden = YES; }
- (NSDictionary *)selectedTitleAttributes
{
  return @{
    NSForegroundColorAttributeName : [UIColor blackPrimaryText],
    NSFontAttributeName : [UIFont bold17]
  };
}

- (NSDictionary *)unselectedTitleAttributes
{
  return @{
    NSForegroundColorAttributeName : [UIColor blackPrimaryText],
    NSFontAttributeName : [UIFont regular17]
  };
}

@end
