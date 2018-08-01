typedef NS_ENUM(NSInteger, MWMCategoryProgress)
{
  MWMCategoryProgressDownloadStarted,
  MWMCategoryProgressDownloadFinished,
  MWMCategoryProgressImportStarted,
  MWMCategoryProgressImportFinished
};

typedef NS_ENUM(NSInteger, MWMCategoryDownloadStatus)
{
  MWMCategoryDownloadStatusForbidden,
  MWMCategoryDownloadStatusNotFound,
  MWMCategoryDownloadStatusNetworkError,
  MWMCategoryDownloadStatusDiskError
};

static NSErrorDomain const kCatalogErrorDomain = @"com.mapswithme.catalog.error";

static NSInteger const kCategoryDownloadFailedCode = -10;
static NSInteger const kCategoryImportFailedCode = -11;

static NSString * const kCategoryDownloadStatusKey = @"kCategoryDownloadStatusKey";

typedef void (^ProgressBlock)(MWMCategoryProgress progress);
typedef void (^CompletionBlock)(UInt64 categoryId, NSError * error);

