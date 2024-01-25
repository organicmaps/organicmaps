#import "PlacePagePreviewData+Core.h"

#include "3party/opening_hours/opening_hours.hpp"

static PlacePageDataSchedule convertOpeningHours(std::string_view rawOH) {
    
  PlacePageDataSchedule schedule;
    
  if (rawOH.empty()) {
    schedule.state = PlacePageDataOpeningHoursUnknown;
    return schedule;
  }

  /// @todo Avoid temporary string when OpeningHours (boost::spirit) will allow string_view.
  osmoh::OpeningHours oh((std::string(rawOH)));
  if (!oh.IsValid()) {
    schedule.state = PlacePageDataOpeningHoursUnknown;
    return schedule;
  }

  if (oh.IsTwentyFourHours()) {
    schedule.state = PlacePageDataOpeningHoursAllDay;
    return schedule;
  }

  auto const t = time(nullptr);
  osmoh::OpeningHours::InfoT info = oh.GetInfo(t);
  switch (info.state) {
    case osmoh::RuleState::Open:
      schedule.state = PlacePageDataOpeningHoursOpen;
      schedule.nextTimeClosed = info.nextTimeClosed;
      break;
          
    case osmoh::RuleState::Closed:
      schedule.state = PlacePageDataOpeningHoursClosed;
      schedule.nextTimeOpen = info.nextTimeOpen;
      break;
          
    case osmoh::RuleState::Unknown:
      schedule.state = PlacePageDataOpeningHoursUnknown;
      break;
  }
  
  return schedule;
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
    _secondaryTitle = rawData.GetSecondaryTitle().empty() ? nil : @(rawData.GetSecondaryTitle().c_str());
    _subtitle = rawData.GetSubtitle().empty() ? nil : @(rawData.GetSubtitle().c_str());
    _coordinates = @(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::LatLonDMS).c_str());
    _address = rawData.GetAddress().empty() ? nil : @(rawData.GetAddress().c_str());
    _isMyPosition = rawData.IsMyPosition();
    _isPopular = rawData.GetPopularity() > 0;
    _schedule = convertOpeningHours(rawData.GetOpeningHours());
    _hotelType = convertHotelType(rawData.GetHotelType());
  }
  return self;
}

@end
