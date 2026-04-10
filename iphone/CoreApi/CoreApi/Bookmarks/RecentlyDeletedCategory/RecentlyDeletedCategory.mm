#import "RecentlyDeletedCategory+Core.h"

#include <platform/platform_ios.h>
#include <map/bookmark_helpers.hpp>

#include "base/logging.hpp"

@implementation RecentlyDeletedCategory

- (instancetype)initTitle:(NSString *)title fileURL:(NSURL *)fileURL deletionDate:(NSDate *)deletionDate
{
  self = [super init];
  if (self)
  {
    _title = title;
    _fileURL = fileURL;
    _deletionDate = deletionDate;
  }
  return self;
}

@end

@implementation RecentlyDeletedCategory (Core)

- (instancetype)initWithCategoryData:(kml::CategoryData)data filePath:(std::string const &)filePath
{
  self = [super init];
  if (self)
  {
    auto const name = GetPreferredBookmarkStr(data.m_name);
    _title = [NSString stringWithCString:name.c_str() encoding:NSUTF8StringEncoding];
    auto const pathString = [NSString stringWithCString:filePath.c_str() encoding:NSUTF8StringEncoding];
    _fileURL = [NSURL fileURLWithPath:pathString];
    auto const creationTime = Platform::GetFileCreationTime(filePath);
    if (creationTime > 0)
    {
      _deletionDate = [NSDate dateWithTimeIntervalSince1970:static_cast<NSTimeInterval>(creationTime)];
    }
    else
    {
      LOG(LWARNING, ("GetFileCreationTime failed for", filePath, "- using current time as deletion date"));
      _deletionDate = [NSDate date];
    }
  }
  return self;
}

@end
