#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageButtonCell.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "Statistics.h"
#import "UIColor+MapsMeColor.h"

@interface MWMPlacePageButtonCell ()

@property (weak, nonatomic) MWMPlacePage * placePage;
@property (weak, nonatomic) IBOutlet UIButton * titleButton;
@property (nonatomic) MWMPlacePageCellType type;

@end

@implementation MWMPlacePageButtonCell

- (void)config:(MWMPlacePage *)placePage forType:(MWMPlacePageCellType)type
{
  self.placePage = placePage;
  switch (type)
  {
  case MWMPlacePageCellTypeAddBusinessButton:
    [self.titleButton setTitle:L(@"placepage_add_business_button") forState:UIControlStateNormal];
    break;
  case MWMPlacePageCellTypeEditButton:
    [self.titleButton setTitle:L(@"edit_place") forState:UIControlStateNormal];
    break;
  case MWMPlacePageCellTypeAddPlaceButton:
    [self.titleButton setTitle:L(@"placepage_add_place_button") forState:UIControlStateNormal];
    break;
  case MWMPlacePageCellTypeBookingMore:
    [self.titleButton setTitle:L(@"placepage_booking_more") forState:UIControlStateNormal];
    break;
  default:
    NSAssert(false, @"Invalid place page cell type!");
    break;
  }
  self.type = type;
}

- (IBAction)buttonTap
{
  switch (self.type)
  {
  case MWMPlacePageCellTypeEditButton:
    [Statistics logEvent:kStatEventName(kStatPlacePage, kStatEdit)];
    [self.placePage editPlace];
    break;
  case MWMPlacePageCellTypeAddBusinessButton:
    [Statistics logEvent:kStatEditorAddClick withParameters:@{kStatValue : kStatPlacePage}];
    [self.placePage addBusiness];
    break;
  case MWMPlacePageCellTypeAddPlaceButton:
    [Statistics logEvent:kStatEditorAddClick withParameters:@{kStatValue : kStatPlacePageNonBuilding}];
    [self.placePage addPlace];
    break;
  case MWMPlacePageCellTypeBookingMore:
  {
    NSMutableDictionary * stat = [@{kStatProvider : kStatBooking} mutableCopy];
    LocationManager * lm = MapsAppDelegate.theApp.locationManager;
    if (lm.lastLocationIsValid)
    {
      CLLocation * loc = lm.lastLocation;
      stat[kStatLat] = @(loc.coordinate.latitude);
      stat[kStatLon] = @(loc.coordinate.longitude);
    }
    else
    {
      stat[kStatLat] = @0;
      stat[kStatLon] = @0;
    }
    MWMPlacePageEntity * en = self.placePage.manager.entity;
    auto const latLon = en.latlon;
    stat[kStatHotel] = @{kStatName : en.title, kStatLat : @(latLon.lat), kStatLon : @(latLon.lon)};
    [Statistics logEvent:kPlacePageHotelDetails withParameters:stat];
    [self.placePage bookingMore];
    break;
  }
  default:
    NSAssert(false, @"Incorrect cell type!");
    break;
  }
}

@end
