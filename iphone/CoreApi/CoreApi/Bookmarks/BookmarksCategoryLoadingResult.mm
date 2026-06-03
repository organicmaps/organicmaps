#import "BookmarksCategoryLoadingResult+Core.h"

static BookmarksCategoryLoadingSource ConvertCategoryLoadingSource(BookmarkManager::CategoryLoadingSource source)
{
  switch (source)
  {
  case BookmarkManager::CategoryLoadingSource::Initial: return BookmarksCategoryLoadingSourceInitial;
  case BookmarkManager::CategoryLoadingSource::Import: return BookmarksCategoryLoadingSourceImport;
  case BookmarkManager::CategoryLoadingSource::Reload: return BookmarksCategoryLoadingSourceReload;
  }
}

static MWMGroupIDCollection ConvertGroupIds(kml::GroupIdCollection const & groupIds)
{
  NSMutableArray<NSNumber *> * result = [NSMutableArray arrayWithCapacity:groupIds.size()];
  for (auto const groupId : groupIds)
    [result addObject:@(groupId)];
  return result;
}

@interface BookmarksCategoryLoadingResult ()

- (instancetype)initWithSource:(BookmarksCategoryLoadingSource)source
                      groupIds:(MWMGroupIDCollection)groupIds
                       success:(BOOL)success
                       fileURL:(NSURL * _Nullable)fileURL
                 temporaryFile:(BOOL)temporaryFile;

@end

@implementation BookmarksCategoryLoadingResult

- (instancetype)initWithSource:(BookmarksCategoryLoadingSource)source
                      groupIds:(MWMGroupIDCollection)groupIds
                       success:(BOOL)success
                       fileURL:(NSURL * _Nullable)fileURL
                 temporaryFile:(BOOL)temporaryFile
{
  self = [super init];
  if (self)
  {
    _source = source;
    _groupIds = [groupIds copy];
    _success = success;
    _fileURL = [fileURL copy];
    _temporaryFile = temporaryFile;
  }
  return self;
}

@end

@implementation BookmarksCategoryLoadingResult (Core)

- (instancetype)initWithCoreResult:(BookmarkManager::CategoryLoadingResult const &)result
{
  NSString * filePath = result.m_filePath.empty() ? nil : [NSString stringWithUTF8String:result.m_filePath.c_str()];
  NSURL * fileURL = filePath == nil ? nil : [NSURL fileURLWithPath:filePath];
  return [self initWithSource:ConvertCategoryLoadingSource(result.m_source)
                     groupIds:ConvertGroupIds(result.m_groupIds)
                      success:result.m_success
                      fileURL:fileURL
                temporaryFile:result.m_isTemporaryFile];
}

@end
