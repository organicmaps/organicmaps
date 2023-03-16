#import "MWMSearchCommonCell.h"
#import "CLLocation+Mercator.h"
#import "MWMLocationManager.h"
#import "SwiftBridge.h"

#include "map/place_page_info.hpp"

#include "geometry/mercator.hpp"

#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"


@interface MWMSearchCommonCell ()

@property(weak, nonatomic) IBOutlet UILabel * distanceLabel;
@property(weak, nonatomic) IBOutlet UILabel * infoLabel;
@property(weak, nonatomic) IBOutlet UILabel * locationLabel;
@property(weak, nonatomic) IBOutlet UILabel * openLabel;
@property(weak, nonatomic) IBOutlet UIView * popularView;

@end

@implementation MWMSearchCommonCell

- (void)config:(search::Result const &)result
    productInfo:(search::ProductInfo const &)productInfo
    localizedTypeName:(NSString *)localizedTypeName
{
  [super config:result localizedTypeName:localizedTypeName];

  self.locationLabel.text = @(result.GetAddress().c_str());
  [self.locationLabel sizeToFit];

  int const starsCount = std::min(7, result.GetStarsCount());
  NSString * cuisine = @(result.GetCuisine().c_str()).capitalizedString;
  NSString * airportIata = @(result.GetAirportIata().c_str());
  NSString * roadShields = @(result.GetRoadShields().c_str());
  NSString * brand  = @"";
  if (!result.GetBrand().empty())
    brand = @(platform::GetLocalizedBrandName(result.GetBrand()).c_str());
  
  NSString * description = @"";

  if (result.IsHotel() && starsCount > 0)
  {
    static NSString * sevenStars = [NSString stringWithUTF8String:"★★★★★★★"];
    description = [sevenStars substringToIndex:starsCount];
  }
  else if (airportIata.length > 0)
    description = airportIata;
  else if (roadShields.length > 0)
    description = roadShields;
  else if (brand.length > 0 && cuisine.length > 0)
    description = [NSString stringWithFormat:@"%@ • %@", brand, cuisine];
  else if (brand.length > 0)
    description = brand;
  else if (cuisine.length > 0)
    description = cuisine;
  
  if ([description length] == 0)
    self.infoLabel.text = localizedTypeName;
  else
    self.infoLabel.text = [NSString stringWithFormat:@"%@ • %@", localizedTypeName, description];

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

  /// @todo Restore "TOP" badge in future, when popularity will be available.
  //self.popularView.hidden = result.GetRankingInfo().m_popularity == 0;
  self.popularView.hidden = YES;
  
  switch (result.IsOpenNow())
  {
    case osm::Yes:
    {
      int const minutes = result.GetMinutesUntilClosed();
      if (minutes < 60) // less than 1 hour
      {
        self.openLabel.textColor = UIColor.systemYellowColor;
        NSString *time = [NSString stringWithFormat: @"%d %@", minutes, L(@"minute")];
        self.openLabel.text = [NSString stringWithFormat: L(@"closes_in"), time];
      }
      else
      {
        self.openLabel.textColor = UIColor.systemGreenColor;
        self.openLabel.text = L(@"editor_time_open");
      }
      self.openLabel.hidden = false;
      break;
    }
      
    case osm::No:
    {
      self.openLabel.textColor = UIColor.systemRedColor;
      int const minutes = result.GetMinutesUntilOpen();
      if (minutes < 60) // less than 1 hour
      {
        NSString *time = [NSString stringWithFormat: @"%d %@", minutes, L(@"minute")];
        self.openLabel.text = [NSString stringWithFormat: L(@"opens_in"), time];
      }
      else
      {
        self.openLabel.text = L(@"closed");
      }
      self.openLabel.hidden = false;
      break;
    }
      
    case osm::Unknown:
    {
      self.openLabel.hidden = true;
      break;
    }
  }

  [self setStyleAndApply: @"Background"];
}

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
