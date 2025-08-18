#import "RecentlyDeletedCategory+Core.h"

#include <platform/platform_ios.h>
#include <map/bookmark_helpers.hpp>

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
    NSTimeInterval creationTime = Platform::GetFileCreationTime(filePath);
    _deletionDate = [NSDate dateWithTimeIntervalSince1970:creationTime];
  }
  return self;
}

@end
