#import "MWMSearchCommonCell.h"
#import "CLLocation+Mercator.h"
#import "MWMLocationManager.h"

#include "geometry/mercator.hpp"
#include "platform/measurement_utils.hpp"

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

@end

@implementation MWMSearchCommonCell

- (void)config:(search::Result const &)result
     isLocalAds:(BOOL)isLocalAds
    isAvailable:(BOOL)isAvailable
{
  [super config:result];
  self.typeLabel.text = @(result.GetFeatureTypeName().c_str()).capitalizedString;
  auto const & ratingStr = result.GetHotelRating();
  self.ratingLabel.text =
      ratingStr.empty() ? @"" : [NSString stringWithFormat:L(@"place_page_booking_rating"),
                                                            ratingStr.c_str()];
  self.priceLabel.text = @(result.GetHotelApproximatePricing().c_str());
  self.locationLabel.text = @(result.GetAddress().c_str());
  [self.locationLabel sizeToFit];

  self.availableTypeOffset.priority = UILayoutPriorityDefaultHigh;
  self.availableView.hidden = !isAvailable;
  self.sideAvailableMarker.hidden = !isAvailable;

  NSUInteger const starsCount = result.GetStarsCount();
  NSString * cuisine = @(result.GetCuisine().c_str());
  if (starsCount > 0)
    [self setInfoRating:starsCount];
  else if (cuisine.length > 0)
    [self setInfoText:cuisine.capitalizedString];
  else
    [self clearInfo];

  self.closedView.hidden = (result.IsOpenNow() != osm::No);
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

  if (isLocalAds)
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
