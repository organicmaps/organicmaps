#import "PlacePageInfoData+Core.h"

#import "OpeningHours.h"

#import <CoreApi/StringUtils.h>

#include "map/place_page_info.hpp"

using namespace place_page;
using namespace osm;

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
          _openingHoursString = ToNSString(rawData.GetOpeningHours());
          _openingHours = [[OpeningHours alloc] initWithRawString:_openingHoursString
                                                     localization:localization];
          break;
        case Props::Phone: {
          _phone = ToNSString(rawData.GetPhone());
          NSString *filteredDigits = [[_phone componentsSeparatedByCharactersInSet:
                                       [[NSCharacterSet decimalDigitCharacterSet] invertedSet]]
                                      componentsJoinedByString:@""];
          NSString *resultNumber = [_phone hasPrefix:@"+"] ? [NSString stringWithFormat:@"+%@", filteredDigits] : filteredDigits;
          _phoneUrl = [NSURL URLWithString:[NSString stringWithFormat:@"tel://%@", resultNumber]];
          break;
        }
        case Props::Website:
          _website = ToNSString(rawData.GetWebsite());
          break;
        case Props::Email:
          _email = ToNSString(rawData.GetEmail());
          break;
        case Props::ContactFacebook:
          _facebook = ToNSString(rawData.GetFacebookPage());
          break;
        case Props::ContactInstagram:
          _instagram = ToNSString(rawData.GetInstagramPage());
          break;
        case Props::ContactTwitter:
          _twitter = ToNSString(rawData.GetTwitterPage());
          break;
        case Props::ContactVk:
          _vk = ToNSString(rawData.GetVkPage());
          break;
        case Props::Cuisine:
          _cuisine = @(strings::JoinStrings(rawData.GetLocalizedCuisines(), Info::kSubtitleSeparator).c_str());
          break;
        case Props::Operator:
          _ppOperator = ToNSString(rawData.GetOperator());
          break;
        case Props::Internet:
          _wifiAvailable = (rawData.GetInternet() == osm::Internet::No)
              ? NSLocalizedString(@"no_available", nil) : NSLocalizedString(@"yes_available", nil);
          break;
        default:
          break;
      }
    }

    _address = rawData.GetAddress().empty() ? nil : @(rawData.GetAddress().c_str());
    _rawCoordinates = @(rawData.GetFormattedCoordinate(true).c_str());
    _formattedCoordinates = @(rawData.GetFormattedCoordinate(false).c_str());
  }
  return self;
}

@end
