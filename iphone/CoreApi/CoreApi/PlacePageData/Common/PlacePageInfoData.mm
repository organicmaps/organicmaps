#import "PlacePageInfoData+Core.h"

#import "OpeningHours.h"

#include "map/place_page_info.hpp"

using namespace place_page;
using namespace osm;

static PlacePageDataLocalAdsStatus convertLocalAdsStatus(LocalAdsStatus status) {
  switch (status) {
    case LocalAdsStatus::NotAvailable:
      return PlacePageDataLocalAdsStatusNotAvailable;
    case LocalAdsStatus::Candidate:
      return PlacePageDataLocalAdsStatusCandidate;
    case LocalAdsStatus::Customer:
      return PlacePageDataLocalAdsStatusCustomer;
    case LocalAdsStatus::Hidden:
      return PlacePageDataLocalAdsStatusHidden;
  }
}

@implementation PlacePageInfoData

@end

@implementation PlacePageInfoData (Core)

- (instancetype)initWithRawData:(Info const &)rawData ohLocalization:(id<IOpeningHoursLocalization>)localization {
  self = [super init];
  if (self) {
    auto availableProperties = rawData.AvailableProperties();
    for (auto property : availableProperties) {
      switch (property) {
        case Props::OpeningHours:
          _openingHoursString = @(rawData.GetOpeningHours().c_str());
          _openingHours = [[OpeningHours alloc] initWithRawString:@(rawData.GetOpeningHours().c_str())
                                                     localization:localization];
          break;
        case Props::Phone: {
          _phone = @(rawData.GetPhone().c_str());
          NSString *filteredDigits = [[_phone componentsSeparatedByCharactersInSet:
                                       [[NSCharacterSet decimalDigitCharacterSet] invertedSet]]
                                      componentsJoinedByString:@""];
          NSString *resultNumber = [_phone hasPrefix:@"+"] ? [NSString stringWithFormat:@"+%@", filteredDigits] : filteredDigits;
          _phoneUrl = [NSURL URLWithString:[NSString stringWithFormat:@"tel://%@", resultNumber]];
          break;
        }
        case Props::Website:
          if (rawData.GetSponsoredType() != SponsoredType::Booking) {
            _website = @(rawData.GetWebsite().c_str());
          }
          break;
        case Props::Email:
            _email = @(rawData.GetEmail().c_str());
            break;
        case Props::Cuisine:
            _cuisine = @(strings::JoinStrings(rawData.GetLocalizedCuisines(), Info::kSubtitleSeparator).c_str());
            break;
        case Props::Operator:
            _ppOperator = @(rawData.GetOperator().c_str());
            break;
        case Props::Internet:
          _wifiAvailable = YES;
            break;
        default:
          break;
      }
    }

    _address = rawData.GetAddress().empty() ? nil : @(rawData.GetAddress().c_str());
    _rawCoordinates = @(rawData.GetFormattedCoordinate(true).c_str());
    _formattedCoordinates = @(rawData.GetFormattedCoordinate(false).c_str());
    _localAdsStatus = convertLocalAdsStatus(rawData.GetLocalAdsStatus());
    _localAdsUrl = rawData.GetLocalAdsUrl().empty() ? nil : @(rawData.GetLocalAdsUrl().c_str());
  }
  return self;
}

@end
