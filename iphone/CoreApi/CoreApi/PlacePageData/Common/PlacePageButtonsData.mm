#import "PlacePageButtonsData+Core.h"

@implementation PlacePageButtonsData

@end

@implementation PlacePageButtonsData (Core)

- (instancetype)initWithRawData:(place_page::Info const &)rawData {
  self = [super init];
  if (self) {
    _showAddPlace = rawData.ShouldShowAddPlace();
    _showEditPlace = rawData.ShouldShowEditPlace();
    _enableAddPlace = rawData.ShouldEnableAddPlace();
    _enableEditPlace = rawData.ShouldEnableEditPlace();
    if (_showAddPlace || _showEditPlace || _enableAddPlace || _enableEditPlace)
      return self;
  }
  return nil;
}

@end
