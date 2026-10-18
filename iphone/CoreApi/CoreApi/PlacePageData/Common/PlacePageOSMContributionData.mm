#import "PlacePageOSMContributionData+Core.h"

@implementation PlacePageOSMContributionData

@end

@implementation PlacePageOSMContributionData (Core)

- (instancetype _Nullable)initWithRawData:(place_page::Info const &)rawData
{
  self = [super init];
  if (self)
  {
    if (!rawData.ShouldShowAddPlace() && !rawData.ShouldShowAddBusiness() && !rawData.ShouldShowEditPlace())
      return nil;

    _showAddPlace = rawData.ShouldShowAddPlace() || rawData.ShouldShowAddBusiness();
    _showEditPlace = rawData.ShouldShowEditPlace();
    _canEditPlace = rawData.CanEditPlace();
  }
  return self;
}

@end
