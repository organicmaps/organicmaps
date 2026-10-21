#import "PlacePageOSMContributionData+Core.h"

@implementation PlacePageOSMContributionData

@end

@implementation PlacePageOSMContributionData (Core)

- (instancetype _Nullable)initWithRawData:(place_page::Info const &)rawData mapStatus:(storage::Status)mapStatus
{
  self = [super init];
  if (self)
  {
    if (!rawData.ShouldShowAddPlace() && !rawData.ShouldShowAddBusiness() && !rawData.ShouldShowEditPlace())
      return nil;

    _showAddPlace = rawData.ShouldShowAddPlace() || rawData.ShouldShowAddBusiness();
    _showEditPlace = rawData.ShouldShowEditPlace();
    _canEditPlace = rawData.CanEditPlace();
    using storage::Status;
    switch (mapStatus)
    {
    case Status::Undefined:
    case Status::OnDisk: _state = PlacePageOSMContributionStateCanAddOrEditPlace; break;
    case Status::Downloading:
    case Status::Applying:
    case Status::InQueue: _state = PlacePageOSMContributionStateMapIsDownloading; break;
    case Status::NotDownloaded:
    case Status::DownloadFailed:
    case Status::UnknownError:
    case Status::OnDiskOutOfDate:
    case Status::OutOfMemFailed:
      _state =
          _canEditPlace ? PlacePageOSMContributionStateCanAddOrEditPlace : PlacePageOSMContributionStateShouldUpdateMap;
      break;
    }
    return self;
  }
  return nil;
}

@end
