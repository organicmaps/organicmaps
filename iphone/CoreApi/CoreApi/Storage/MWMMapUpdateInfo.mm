#import "MWMMapUpdateInfo+Core.h"

@implementation MWMMapUpdateInfo

@end

@implementation MWMMapUpdateInfo (Core)

- (instancetype)initWithUpdateInfo:(storage::Storage::UpdateInfo const &)updateInfo
{
  self = [super init];
  if (self)
  {
    _numberOfFiles = updateInfo.m_numberOfMwmFilesToUpdate;
    _updateSize = updateInfo.m_totalDownloadSizeInBytes;
    _differenceSize = updateInfo.m_sizeDifference;
  }
  return self;
}

@end
