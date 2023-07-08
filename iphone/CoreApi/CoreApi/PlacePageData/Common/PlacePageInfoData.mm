#import "PlacePageInfoData+Core.h"

#import "OpeningHours.h"

#import <CoreApi/StringUtils.h>

#include "indexer/validate_and_format_contacts.hpp"

#include "map/place_page_info.hpp"

using namespace place_page;
using namespace osm;

@implementation PlacePageInfoData

@end

@implementation PlacePageInfoData (Core)

- (instancetype)initWithRawData:(Info const &)rawData ohLocalization:(id<IOpeningHoursLocalization>)localization {
  self = [super init];
  if (self)
  {
    auto const cuisines = rawData.FormatCuisines();
    if (!cuisines.empty())
      _cuisine = ToNSString(cuisines);

    /// @todo Refactor PlacePageInfoData to have a map of simple string properties.
    using MetadataID = MapObject::MetadataID;
    rawData.ForEachMetadataReadable([&](MetadataID metaID, std::string const & value)
    {
      switch (metaID)
      {
        case MetadataID::FMD_OPEN_HOURS:
          _openingHoursString = ToNSString(value);
          _openingHours = [[OpeningHours alloc] initWithRawString:_openingHoursString
                                                     localization:localization];
          break;
        case MetadataID::FMD_PHONE_NUMBER:
        {
          _phone = ToNSString(value);
          NSString *filteredDigits = [[_phone componentsSeparatedByCharactersInSet:
                                       [[NSCharacterSet decimalDigitCharacterSet] invertedSet]]
                                      componentsJoinedByString:@""];
          NSString *resultNumber = [_phone hasPrefix:@"+"] ? [NSString stringWithFormat:@"+%@", filteredDigits] : filteredDigits;
          _phoneUrl = [NSURL URLWithString:[NSString stringWithFormat:@"tel://%@", resultNumber]];
          break;
        }
        case MetadataID::FMD_WEBSITE: _website = ToNSString(value); break;
        case MetadataID::FMD_WIKIPEDIA: _wikipedia = ToNSString(value); break;
        case MetadataID::FMD_WIKIMEDIA_COMMONS: _wikimediaCommons = ToNSString(value); break;
        case MetadataID::FMD_EMAIL:
          _email = ToNSString(value);
          _emailUrl = [NSURL URLWithString:[NSString stringWithFormat:@"mailto:%@", _email]];
          break;
        case MetadataID::FMD_CONTACT_FACEBOOK: _facebook = ToNSString(value); break;
        case MetadataID::FMD_CONTACT_INSTAGRAM: _instagram = ToNSString(value); break;
        case MetadataID::FMD_CONTACT_TWITTER: _twitter = ToNSString(value); break;
        case MetadataID::FMD_CONTACT_VK: _vk = ToNSString(value); break;
        case MetadataID::FMD_CONTACT_LINE: _line = ToNSString(value); break;
        case MetadataID::FMD_OPERATOR: _ppOperator = ToNSString(value); break;
        case MetadataID::FMD_INTERNET:
          _wifiAvailable = (rawData.GetInternet() == osm::Internet::No)
              ? NSLocalizedString(@"no_available", nil) : NSLocalizedString(@"yes_available", nil);
          break;
        case MetadataID::FMD_LEVEL: _level = ToNSString(value); break;
        default:
          break;
      }
    });

    _address = rawData.GetAddress().empty() ? nil : @(rawData.GetAddress().c_str());
    _coordFormats = @[@(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::LatLonDMS).c_str()),
                      @(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::LatLonDecimal).c_str()),
                      @(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::OLCFull).c_str()),
                      @(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::OSMLink).c_str()),
                      @(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::UTM).c_str()),
                      @(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::MGRS).c_str())];
  }
  return self;
}

@end
