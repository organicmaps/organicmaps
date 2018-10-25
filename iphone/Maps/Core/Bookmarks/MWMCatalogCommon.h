typedef NS_ENUM(NSInteger, MWMCategoryProgress)
{
  MWMCategoryProgressDownloadStarted,
  MWMCategoryProgressDownloadFinished,
  MWMCategoryProgressImportStarted,
  MWMCategoryProgressImportFinished,
  MWMCategoryProgressUploadStarted
};

typedef NS_ENUM(NSInteger, MWMCategoryDownloadStatus)
{
  MWMCategoryDownloadStatusNeedAuth,
  MWMCategoryDownloadStatusNeedPayment,
  MWMCategoryDownloadStatusNotFound,
  MWMCategoryDownloadStatusNetworkError,
  MWMCategoryDownloadStatusDiskError
};

typedef NS_ENUM(NSInteger, MWMCategoryUploadStatus)
{
  MWMCategoryUploadStatusNetworkError,
  MWMCategoryUploadStatusServerError,
  MWMCategoryUploadStatusAuthError,
  MWMCategoryUploadStatusMalformedData,
  MWMCategoryUploadStatusAccessError,
  MWMCategoryUploadStatusInvalidCall
};

static NSErrorDomain const kCatalogErrorDomain = @"com.mapswithme.catalog.error";

static NSInteger const kCategoryDownloadFailedCode = -10;
static NSInteger const kCategoryImportFailedCode = -11;
static NSInteger const kCategoryUploadFailedCode = -12;

static NSString * const kCategoryDownloadStatusKey = @"kCategoryDownloadStatusKey";
static NSString * const kCategoryUploadStatusKey = @"kCategoryUploadStatusKey";

typedef void (^ProgressBlock)(MWMCategoryProgress progress);
typedef void (^DownloadCompletionBlock)(UInt64 categoryId, NSError * error);
typedef void (^UploadCompletionBlock)(NSURL * url, NSError * error);

