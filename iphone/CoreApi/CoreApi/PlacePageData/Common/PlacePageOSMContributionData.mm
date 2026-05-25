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

    BOOL shouldShowAddPlace = rawData.ShouldShowAddPlace() || rawData.ShouldShowAddBusiness();
    BOOL shouldShowEditPlace = rawData.ShouldShowEditPlace();
    BOOL canEditPlace = rawData.CanEditPlace();
    switch (mapAttributes.nodeStatus)
    {
    case MWMMapNodeStatusUndefined:
    case MWMMapNodeStatusOnDisk:
      _state = PlacePageOSMContributionStateCanAddOrEditPlace;
      _showAddPlace = shouldShowAddPlace;
      _showEditPlace = shouldShowEditPlace;
      _canEditPlace = canEditPlace;
      break;
    case MWMMapNodeStatusDownloading:
    case MWMMapNodeStatusApplying:
    case MWMMapNodeStatusInQueue: _state = PlacePageOSMContributionStateMapIsDownloading; break;
    case MWMMapNodeStatusError:
    case MWMMapNodeStatusNotDownloaded:
    case MWMMapNodeStatusOnDiskOutOfDate:
    case MWMMapNodeStatusPartly:
      BOOL needsToUpdateMap = !canEditPlace;
      _state = needsToUpdateMap ? PlacePageOSMContributionStateShouldUpdateMap
                                : PlacePageOSMContributionStateCanAddOrEditPlace;
      _showAddPlace = !needsToUpdateMap && shouldShowAddPlace;
      _showEditPlace = !needsToUpdateMap && shouldShowEditPlace;
      _canEditPlace = canEditPlace;
      break;
    }
    return self;
  }
  return nil;
}

@end
