#import "MWMSearchCommonCell.h"
#import "CLLocation+Mercator.h"
#import "MWMCommon.h"
#import "MWMLocationManager.h"
#import "MapsAppDelegate.h"

#include "Framework.h"

#include "geometry/mercator.hpp"
#include "platform/measurement_utils.hpp"

@interface MWMSearchCommonCell ()

@property(weak, nonatomic) IBOutlet UILabel * typeLabel;
@property(weak, nonatomic) IBOutlet UIView * infoView;
@property(weak, nonatomic) IBOutlet UILabel * infoLabel;
@property(weak, nonatomic) IBOutlet UIView * infoRatingView;
@property(nonatomic) IBOutletCollection(UIImageView) NSArray * infoRatingStars;
@property(weak, nonatomic) IBOutlet UILabel * locationLabel;
@property(weak, nonatomic) IBOutlet UILabel * distanceLabel;
@property(weak, nonatomic) IBOutlet UILabel * ratingLabel;
@property(weak, nonatomic) IBOutlet UILabel * priceLabel;

@property(weak, nonatomic) IBOutlet UIView * closedView;

@end

@implementation MWMSearchCommonCell

- (void)config:(search::Result const &)result isLocalAds:(BOOL)isLocalAds
{
  [super config:result];
  self.typeLabel.text = @(result.GetFeatureType().c_str()).capitalizedString;
  auto const & ratingStr = result.GetHotelRating();
  self.ratingLabel.text =
      ratingStr.empty() ? @"" : [NSString stringWithFormat:L(@"place_page_booking_rating"),
                                                            ratingStr.c_str()];
  self.priceLabel.text = @(result.GetHotelApproximatePricing().c_str());
  self.locationLabel.text = @(result.GetAddress().c_str());
  [self.locationLabel sizeToFit];

  NSUInteger const starsCount = result.GetStarsCount();
  NSString * cuisine = @(result.GetCuisine().c_str());
  if (starsCount > 0)
    [self setInfoRating:starsCount];
  else if (cuisine.length > 0)
    [self setInfoText:cuisine.capitalizedString];
  else
    [self clearInfo];

  switch (result.IsOpenNow())
  {
    case osm::Unknown:
    // TODO: Correctly handle Open Now = YES value (show "OPEN" mark).
    case osm::Yes: self.closedView.hidden = YES; break;
    case osm::No: self.closedView.hidden = NO; break;
    }

    if (result.HasPoint())
    {
      string distanceStr;
      CLLocation * lastLocation = [MWMLocationManager lastLocation];
      if (lastLocation)
      {
        double const dist =
            MercatorBounds::DistanceOnEarth(lastLocation.mercator, result.GetFeatureCenter());
        measurement_utils::FormatDistance(dist, distanceStr);
      }
      self.distanceLabel.text = @(distanceStr.c_str());
  }

  self.backgroundColor = isLocalAds ? [UIColor bannerBackground] : [UIColor white];
}

- (void)setInfoText:(NSString *)infoText
{
  self.infoView.hidden = NO;
  self.infoLabel.hidden = NO;
  self.infoRatingView.hidden = YES;
  self.infoLabel.text = infoText;
}

- (void)setInfoRating:(NSUInteger)infoRating
{
  self.infoView.hidden = NO;
  self.infoRatingView.hidden = NO;
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
    NSForegroundColorAttributeName : UIColor.blackPrimaryText,
    NSFontAttributeName : UIFont.bold17
  };
}

- (NSDictionary *)unselectedTitleAttributes
{
  return @{
    NSForegroundColorAttributeName : UIColor.blackPrimaryText,
    NSFontAttributeName : UIFont.regular17
  };
}

@end
