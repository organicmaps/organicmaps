#include <CoreApi/Framework.h>
#import "MWMMapNodeAttributes+Core.h"

static MWMMapNodeStatus convertStatus(storage::NodeStatus status)
{
  switch (status)
  {
  case storage::NodeStatus::Undefined: return MWMMapNodeStatusUndefined;
  case storage::NodeStatus::Downloading: return MWMMapNodeStatusDownloading;
  case storage::NodeStatus::Applying: return MWMMapNodeStatusApplying;
  case storage::NodeStatus::InQueue: return MWMMapNodeStatusInQueue;
  case storage::NodeStatus::Error: return MWMMapNodeStatusError;
  case storage::NodeStatus::OnDiskOutOfDate: return MWMMapNodeStatusOnDiskOutOfDate;
  case storage::NodeStatus::OnDisk: return MWMMapNodeStatusOnDisk;
  case storage::NodeStatus::NotDownloaded: return MWMMapNodeStatusNotDownloaded;
  case storage::NodeStatus::Partly: return MWMMapNodeStatusPartly;
  }
}

@interface MWMCountryIdAndName ()

@property(nonatomic, copy) NSString * countryId;
@property(nonatomic, copy) NSString * countryName;

@end

@implementation MWMCountryIdAndName

- (instancetype)initWithCountryId:(NSString *)countryId name:(NSString *)countryName
{
  self = [super init];
  if (self)
  {
    _countryId = countryId;
    _countryName = countryName;
  }
  return self;
}

- (instancetype)initWithCountryAndName:(storage::CountryIdAndName const &)countryAndName
{
  self = [super init];
  if (self)
  {
    _countryId = @(countryAndName.m_id.c_str());
    _countryName = @(countryAndName.m_localName.c_str());
  }
  return self;
}

@end

@implementation MWMMapNodeAttributes

@end

@implementation MWMMapNodeAttributes (Core)

- (instancetype)initWithCoreAttributes:(storage::NodeAttrs const &)attributes
                             countryId:(NSString *)countryId
                             hasParent:(BOOL)hasParent
                           hasChildren:(BOOL)hasChildren
{
  self = [super init];
  if (self)
  {
    _countryId = [countryId copy];
    _totalMwmCount = attributes.m_mwmCounter;
    _downloadedMwmCount = attributes.m_localMwmCounter;
    _downloadingMwmCount = attributes.m_downloadingMwmCounter - attributes.m_localMwmCounter;
    _totalSize = attributes.m_mwmSize;
    _downloadedSize = attributes.m_localMwmSize;
    _downloadingSize = attributes.m_downloadingMwmSize - attributes.m_localMwmSize;
    _nodeName = @(attributes.m_nodeLocalName.c_str());
    _nodeDescription = @(attributes.m_nodeLocalDescription.c_str());
    _nodeStatus = convertStatus(attributes.m_status);
    _hasChildren = hasChildren;
    _hasParent = hasParent;

    storage::Storage::UpdateInfo updateInfo;
    if (GetFramework().GetStorage().GetUpdateInfo([countryId UTF8String], updateInfo))
      _totalUpdateSizeBytes = updateInfo.m_totalDownloadSizeInBytes;
    else
      _totalUpdateSizeBytes = 0;

    NSMutableArray * parentInfoArray = [NSMutableArray arrayWithCapacity:attributes.m_parentInfo.size()];
    for (auto const & pi : attributes.m_parentInfo)
    {
      MWMCountryIdAndName * cn = [[MWMCountryIdAndName alloc] initWithCountryAndName:pi];
      [parentInfoArray addObject:cn];
    }
    _parentInfo = [parentInfoArray copy];

    NSMutableArray * topmostInfoArray = [NSMutableArray arrayWithCapacity:attributes.m_topmostParentInfo.size()];
    for (auto const & pi : attributes.m_topmostParentInfo)
    {
      MWMCountryIdAndName * cn = [[MWMCountryIdAndName alloc] initWithCountryAndName:pi];
      [topmostInfoArray addObject:cn];
    }
    _topmostParentInfo = topmostInfoArray.count > 0 ? [topmostInfoArray copy] : nil;
  }
  return self;
}

@end
