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
      _showAddPlace = rawData.ShouldShowAddPlace() || rawData.ShouldShowAddBusiness();
      _showEditPlace = rawData.ShouldShowEditPlace();
      _showUpdateMap = false;

      _enableAddPlace = true;
      _enableEditPlace = true;
      _enableUpdateMap = false;
      break;
    case MWMMapNodeStatusDownloading:
    case MWMMapNodeStatusApplying:
    case MWMMapNodeStatusInQueue:
      _showAddPlace = rawData.ShouldShowAddPlace() || rawData.ShouldShowAddBusiness();
      _showEditPlace = rawData.ShouldShowEditPlace();
      _showUpdateMap = false;

      _enableAddPlace = false;
      _enableEditPlace = false;
      _enableUpdateMap = false;
      break;
    case MWMMapNodeStatusError:
    case MWMMapNodeStatusNotDownloaded:
    case MWMMapNodeStatusOnDiskOutOfDate:
    case MWMMapNodeStatusPartly:
      BOOL needsToUpdateMap = !rawData.ShouldEnableAddPlace() || !rawData.ShouldEnableEditPlace();
      _showAddPlace = !needsToUpdateMap;
      _showEditPlace = !needsToUpdateMap;
      _showUpdateMap = needsToUpdateMap;

      _enableAddPlace = !needsToUpdateMap;
      _enableEditPlace = !needsToUpdateMap;
      _enableUpdateMap = needsToUpdateMap;
      break;
    }
    return self;
  }
  return nil;
}

@end
