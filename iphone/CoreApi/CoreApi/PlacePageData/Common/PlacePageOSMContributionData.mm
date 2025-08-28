#import "MWMMapNodeAttributes.h"
#import "PlacePageOSMContributionData+Core.h"

@implementation PlacePageOSMContributionData

@end

@implementation PlacePageOSMContributionData (Core)

- (instancetype _Nullable)initWithRawData:(place_page::Info const &)rawData
                            mapAttributes:(MWMMapNodeAttributes * _Nonnull)mapAttributes
{
  self = [super init];
  if (self)
  {
    if (!rawData.ShouldShowAddPlace() && !rawData.ShouldShowAddBusiness() && !rawData.ShouldShowEditPlace())
      return nil;

    switch (mapAttributes.nodeStatus)
    {
    case MWMMapNodeStatusUndefined:
    case MWMMapNodeStatusOnDisk:
      _state = PlacePageOSMContributionStateCanAddOrEditPlace;
      _showAddPlace = rawData.ShouldShowAddPlace() || rawData.ShouldShowAddBusiness();
      _showEditPlace = rawData.ShouldShowEditPlace();
      break;
    case MWMMapNodeStatusDownloading:
    case MWMMapNodeStatusApplying:
    case MWMMapNodeStatusInQueue: _state = PlacePageOSMContributionStateMapIsDownloading; break;
    case MWMMapNodeStatusError:
    case MWMMapNodeStatusNotDownloaded:
    case MWMMapNodeStatusOnDiskOutOfDate:
    case MWMMapNodeStatusPartly:
      BOOL needsToUpdateMap = !rawData.ShouldEnableAddPlace() || !rawData.ShouldEnableEditPlace();
      _state = needsToUpdateMap ? PlacePageOSMContributionStateShouldUpdateMap
                                : PlacePageOSMContributionStateCanAddOrEditPlace;
      _showAddPlace = !needsToUpdateMap;
      _showEditPlace = !needsToUpdateMap;
      break;
    }
    return self;
  }
  return nil;
}

@end
