#import "MWMGeoTrackerCore.h"

#include "map/framework_light.hpp"

@interface MWMGeoTrackerZone: NSObject<IMWMGeoTrackerZone>

@property (nonatomic, readwrite) NSString *identifier;

- (instancetype)initWithCampaignFeature:(CampaignFeature const &)feature;

- (CampaignFeature const &)feature;

@end

@implementation MWMGeoTrackerZone {
  CampaignFeature _feature;
}

- (instancetype)initWithCampaignFeature:(CampaignFeature const &)feature {
  self = [super init];
  if (self) {
    _feature = feature;
    _identifier = [NSUUID UUID].UUIDString;
  }

  return self;
}

- (CLLocationDegrees)latitude {
  return _feature.m_lat;
}

- (CLLocationDegrees)longitude {
  return _feature.m_lon;
}

- (CampaignFeature const &)feature {
  return _feature;
}

@end

@implementation MWMGeoTrackerCore

- (NSArray<id<IMWMGeoTrackerZone>> *)geoZonesForLat:(CLLocationDegrees)lat
                                                lon:(CLLocationDegrees)lon
                                           accuracy:(CLLocationAccuracy)accuracy {
  lightweight::Framework f(lightweight::REQUEST_TYPE_LOCAL_ADS_FEATURES);
  auto campainFeatures = f.GetLocalAdsFeatures(lat, lon, accuracy, 20);
  NSMutableArray * result = [NSMutableArray array];
  for (auto const & cf : campainFeatures) {
    [result addObject:[[MWMGeoTrackerZone alloc] initWithCampaignFeature:cf]];
  }
  return [result copy];
}

- (void)logEnterZone:(id<IMWMGeoTrackerZone>)zone location:(CLLocation *)location {
  NSAssert([zone isKindOfClass:MWMGeoTrackerZone.class], @"zone must be of MWMGeoTrackerZone type");
  MWMGeoTrackerZone * geoZone = (MWMGeoTrackerZone *)zone;
  auto feature = geoZone.feature;
  lightweight::Framework f(lightweight::REQUEST_TYPE_LOCAL_ADS_STATISTICS);
  local_ads::Event event(local_ads::EventType::Visit,
                         feature.m_mwmVersion,
                         feature.m_countryId,
                         feature.m_featureIndex,
                         0,
                         local_ads::Clock::now(),
                         location.coordinate.latitude,
                         location.coordinate.longitude,
                         location.horizontalAccuracy);
  f.GetLocalAdsStatistics()->RegisterEvent(std::move(event));
}

@end
