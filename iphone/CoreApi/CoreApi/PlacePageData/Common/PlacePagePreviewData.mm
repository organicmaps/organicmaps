#import "PlacePagePreviewData+Core.h"

#include "3party/opening_hours/opening_hours.hpp"
#include "kml/types.hpp"

static PlacePageDataSchedule convertOpeningHours(std::string_view rawOH)
{
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

@implementation PlacePagePreviewData

@end

@implementation PlacePagePreviewData (Core)

- (instancetype)initWithRawData:(place_page::Info const &)rawData {
  self = [super init];
  if (self) {
    _title = rawData.GetTitle().empty() ? nil : @(rawData.GetTitle().c_str());
    _secondaryTitle = rawData.GetSecondaryTitle().empty() ? nil : @(rawData.GetSecondaryTitle().c_str());
    _subtitle = rawData.GetSubtitle().empty() ? nil : @(rawData.GetSubtitle().c_str());
    _coordinates = @(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::LatLonDMS).c_str());
    _address = rawData.GetAddress().empty() ? nil : @(rawData.GetAddress().c_str());
    _isMyPosition = rawData.IsMyPosition();
    //_isPopular = rawData.GetPopularity() > 0;
    _schedule = convertOpeningHours(rawData.GetOpeningHours());

    if (rawData.IsTrack()) {
      _coordinates = nullptr;
      _address = nullptr;
      _isMyPosition = false;
      _schedule = convertOpeningHours("");
    }
  }
  return self;
}

@end
