#import "MWMBookmarksImportResult+Core.h"

@interface MWMBookmarksImportResult ()

- (instancetype)initWithGroupIds:(MWMGroupIDCollection)groupIds failedFileNames:(NSArray<NSString *> *)failedFileNames;

@end

@implementation MWMBookmarksImportResult

- (instancetype)initWithGroupIds:(MWMGroupIDCollection)groupIds failedFileNames:(NSArray<NSString *> *)failedFileNames
{
  self = [super init];
  if (self)
  {
    _groupIds = [groupIds copy];
    _failedFileNames = [failedFileNames copy];
  }
  return self;
}

@end

@implementation MWMBookmarksImportResult (Core)

- (instancetype)initWithCoreResult:(BookmarkManager::BookmarkImportResult const &)result
{
  NSMutableArray<NSNumber *> * groupIds = [NSMutableArray array];
  NSMutableArray<NSString *> * failedFileNames = [NSMutableArray array];
  for (auto const & sourceResult : result.m_sourceResults)
  {
    for (auto const groupId : sourceResult.m_groupIds)
      [groupIds addObject:@(groupId)];
    for (auto const & failedFileName : sourceResult.m_failedFileNames)
      [failedFileNames addObject:@(failedFileName.c_str())];
  }
  return [self initWithGroupIds:groupIds failedFileNames:failedFileNames];
}

@end
