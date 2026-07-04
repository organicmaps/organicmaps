#import "AltitudeFormatter.h"
#import "DistanceFormatter.h"
#import "DurationFormatter.h"
#import "PlacePagePreviewData+Core.h"
#import "TrackInfo.h"

#include "3party/opening_hours/opening_hours.hpp"

#include <optional>

static PlacePageDataSchedule convertOpeningHours(std::string_view rawOH,
                                                 std::optional<om::tz::TimeZone> const & timeZone)
{
  PlacePageDataSchedule schedule;

  if (rawOH.empty())
  {
    schedule.state = PlacePageDataOpeningHoursUnknown;
    return schedule;
  }

  /// @todo Avoid temporary string when OpeningHours (boost::spirit) will allow string_view.
  osmoh::OpeningHours oh((std::string(rawOH)));
  if (!oh.IsValid())
  {
    schedule.state = PlacePageDataOpeningHoursUnknown;
    return schedule;
  }

  if (oh.IsTwentyFourHours())
  {
    schedule.state = PlacePageDataOpeningHoursAllDay;
    return schedule;
  }

  auto const t = time(nullptr);
  // Evaluate in the POI's local time zone (not the device's), see issue #1642.
  osmoh::OpeningHours::InfoT info = oh.GetInfo(t, timeZone);
  switch (info.state)
  {
  case osmoh::RuleState::Open:
    schedule.state = PlacePageDataOpeningHoursOpen;
    schedule.nextTimeClosed = info.nextTimeClosed;
    break;

  case osmoh::RuleState::Closed:
    schedule.state = PlacePageDataOpeningHoursClosed;
    schedule.nextTimeOpen = info.nextTimeOpen;
    break;

  case osmoh::RuleState::Unknown: schedule.state = PlacePageDataOpeningHoursUnknown; break;
  }

  return schedule;
}

@implementation PlacePagePreviewData

- (instancetype)initWithTrackInfo:(TrackInfo * _Nonnull)trackInfo
{
  self = [super init];
  if (self)
  {
    NSString * kSeparator = @" • ";
    _title = [@[trackInfo.duration, trackInfo.distance] componentsJoinedByString:kSeparator];
  }
  return self;
}

@end

@implementation PlacePagePreviewData (Core)

- (instancetype)initWithRawData:(place_page::Info const &)rawData
{
  self = [super init];
  if (self)
  {
    _title = rawData.GetTitle().empty() ? nil : @(rawData.GetTitle().c_str());
    _secondaryTitle = rawData.GetSecondaryTitle().empty() ? nil : @(rawData.GetSecondaryTitle().c_str());
    _subtitle = rawData.GetSubtitle().empty() ? nil : @(rawData.GetSubtitle().c_str());
    _secondarySubtitle = rawData.GetSecondarySubtitle().empty() ? nil : @(rawData.GetSecondarySubtitle().c_str());

    if (!rawData.IsTrack())
    {
      _coordinates = @(rawData.GetFormattedCoordinate(place_page::CoordinatesFormat::LatLonDMS).c_str());
      _isMyPosition = rawData.IsMyPosition();
      _schedule = convertOpeningHours(rawData.GetOpeningHours(), rawData.GetTimeZone());
    }
  }
  return self;
}

@end
