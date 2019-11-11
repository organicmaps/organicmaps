#import "PlacePageButtonsData+Core.h"

@implementation PlacePageButtonsData

@end

@implementation PlacePageButtonsData (Core)

- (instancetype)initWithRawData:(place_page::Info const &)rawData {
  self = [super init];
  if (self) {
    if (rawData.GetSponsoredType() == place_page::SponsoredType::Booking) {
      _showHotelDescription = YES;
    } else {
      _showAddPlace = rawData.ShouldShowAddPlace();
      _showEditPlace = rawData.ShouldShowEditPlace();
      _showAddBusiness = rawData.ShouldShowAddBusiness();
    }
  }

  if (_showHotelDescription || _showAddPlace || _showEditPlace || _showAddBusiness) {
    return self;
  } else {
    return nil;
  }
}

@end
