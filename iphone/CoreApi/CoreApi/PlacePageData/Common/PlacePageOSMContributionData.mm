#import "MWMMapNodeAttributes.h"
#import "PlacePageOSMContributionData+Core.h"

@implementation PlacePageOSMContributionData

@end

@implementation PlacePageOSMContributionData (Core)

- (instancetype)initWithRawData:(place_page::Info const &)rawData
                  mapAttributes:(MWMMapNodeAttributes * _Nullable)mapAttributes
{
  self = [super init];
  if (self)
  {
    if (!rawData.ShouldShowAddPlace() && !rawData.ShouldShowAddBusiness() && !rawData.ShouldShowEditPlace())
      return nil;

    BOOL needsToUpdateMap = !rawData.ShouldEnableAddPlace() || !rawData.ShouldEnableEditPlace();
    _showAddPlace = !needsToUpdateMap && (rawData.ShouldShowAddPlace() || rawData.ShouldShowAddBusiness());
    _showEditPlace = !needsToUpdateMap && rawData.ShouldShowEditPlace();
    _showUpdateMap = needsToUpdateMap;

    if (mapAttributes)
    {
      switch (mapAttributes.nodeStatus)
      {
      case MWMMapNodeStatusUndefined:
      case MWMMapNodeStatusOnDisk:
      case MWMMapNodeStatusOnDiskOutOfDate:
        if (_showUpdateMap)
          ASSERT_FAIL("Update Maps button shouldn't be displayed when node is in these states");
        break;
      case MWMMapNodeStatusDownloading:
      case MWMMapNodeStatusApplying:
      case MWMMapNodeStatusInQueue: _enableUpdateMap = false; break;
      case MWMMapNodeStatusError:
      case MWMMapNodeStatusNotDownloaded:
      case MWMMapNodeStatusPartly: _enableUpdateMap = true; break;
      }
    }
    return self;
  }
  return nil;
}

@end
