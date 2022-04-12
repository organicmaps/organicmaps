#import "PlacePagePreviewData+Core.h"

#include "3party/opening_hours/opening_hours.hpp"

static PlacePageDataSchedule convertOpeningHours(std::string_view rawOH) {
  if (rawOH.empty())
    return PlacePageDataOpeningHoursUnknown;

  /// @todo Avoid temporary string when OpeningHours (boost::spirit) will allow string_view.
  osmoh::OpeningHours oh((std::string(rawOH)));
  if (!oh.IsValid()) {
    return PlacePageDataOpeningHoursUnknown;
  }
  if (oh.IsTwentyFourHours()) {
    return PlacePageDataOpeningHoursAllDay;
  }

  auto const t = time(nullptr);
  if (oh.IsOpen(t)) {
    return PlacePageDataOpeningHoursOpen;
  }
  if (oh.IsClosed(t)) {
    return PlacePageDataOpeningHoursClosed;
  }

  return PlacePageDataOpeningHoursUnknown;
}

static PlacePageDataHotelType convertHotelType(std::optional<ftypes::IsHotelChecker::Type> hotelType) {
  if (!hotelType.has_value()) {
    return PlacePageDataHotelTypeNone;
  }

  switch (*hotelType) {
    case ftypes::IsHotelChecker::Type::Hotel:
      return PlacePageDataHotelTypeHotel;
    case ftypes::IsHotelChecker::Type::Apartment:
      return PlacePageDataHotelTypeApartment;
    case ftypes::IsHotelChecker::Type::CampSite:
      return PlacePageDataHotelTypeCampSite;
    case ftypes::IsHotelChecker::Type::Chalet:
      return PlacePageDataHotelTypeChalet;
    case ftypes::IsHotelChecker::Type::GuestHouse:
      return PlacePageDataHotelTypeGuestHouse;
    case ftypes::IsHotelChecker::Type::Hostel:
      return PlacePageDataHotelTypeHostel;
    case ftypes::IsHotelChecker::Type::Motel:
      return PlacePageDataHotelTypeMotel;
    case ftypes::IsHotelChecker::Type::Resort:
      return PlacePageDataHotelTypeResort;
    case ftypes::IsHotelChecker::Type::Count:
      return PlacePageDataHotelTypeNone;
  }
}

@implementation PlacePagePreviewData

@end

@implementation PlacePagePreviewData (Core)

- (instancetype)initWithElevationInfo:(ElevationInfo const &)elevationInfo {
  self = [super init];
  if (self) {
     _title = @(elevationInfo.GetName().c_str());
  }
  return self;
}

- (instancetype)initWithRawData:(place_page::Info const &)rawData {
  self = [super init];
  if (self) {
    _title = rawData.GetTitle().empty() ? nil : @(rawData.GetTitle().c_str());
    _subtitle = rawData.GetSubtitle().empty() ? nil : @(rawData.GetSubtitle().c_str());
    _coordinates = rawData.GetFormattedCoordinate(true).empty() ? nil : @(rawData.GetFormattedCoordinate(true).c_str());
    _address = rawData.GetAddress().empty() ? nil : @(rawData.GetAddress().c_str());
    _isMyPosition = rawData.IsMyPosition();
    _isPopular = rawData.GetPopularity() > 0;
    _schedule = convertOpeningHours(rawData.GetOpeningHours());
    _hotelType = convertHotelType(rawData.GetHotelType());
  }
  return self;
}

@end
