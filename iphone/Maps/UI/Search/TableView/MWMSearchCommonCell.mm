#import "MWMSearchCommonCell.h"
#import "CLLocation+Mercator.h"
#import "MWMLocationManager.h"
#import "SwiftBridge.h"

#include "map/place_page_info.hpp"

#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"
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
@property(weak, nonatomic) IBOutlet UILabel * typeLabel;
@property(weak, nonatomic) IBOutlet UIView * closedView;
@property(weak, nonatomic) IBOutlet UIView * infoRatingView;
@property(weak, nonatomic) IBOutlet UIView * infoView;
@property(weak, nonatomic) IBOutlet UIView * popularView;

@end

@implementation MWMSearchCommonCell

- (void)config:(search::Result const &)result
    productInfo:(search::ProductInfo const &)productInfo
    localizedTypeName:(NSString *)localizedTypeName
{
  [super config:result localizedTypeName:localizedTypeName];

  self.typeLabel.text = localizedTypeName;

  self.locationLabel.text = @(result.GetAddress().c_str());
  [self.locationLabel sizeToFit];

  NSUInteger const starsCount = result.GetStarsCount();
  NSString * cuisine = @(result.GetCuisine().c_str()).capitalizedString;
  NSString * airportIata = @(result.GetAirportIata().c_str());
  NSString * roadShields = @(result.GetRoadShields().c_str());
  NSString * brand  = @"";
  if (!result.GetBrand().empty())
    brand = @(platform::GetLocalizedBrandName(result.GetBrand()).c_str());

  if (starsCount > 0)
    [self setInfoRating:starsCount];
  else if (airportIata.length > 0)
    [self setInfoText:airportIata];
  else if (roadShields.length > 0)
    [self setInfoText:roadShields];
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
      auto const localizedUnits = platform::GetLocalizedDistanceUnits();
      distanceInMeters =
          mercator::DistanceOnEarth(lastLocation.mercator, result.GetFeatureCenter());
      std::string distanceStr = measurement_utils::FormatDistanceWithLocalization(distanceInMeters,
                                                                                  localizedUnits.m_high,
                                                                                  localizedUnits.m_low);
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

  [self setStyleAndApply: @"Background"];
}

- (void)setInfoText:(NSString *)infoText
{
  self.infoView.hidden = NO;
  self.infoLabel.hidden = NO;
  self.infoLabel.text = infoText;
}

- (void)setInfoRating:(NSUInteger)infoRating
{
  self.infoView.hidden = NO;
  self.infoLabel.hidden = YES;
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
