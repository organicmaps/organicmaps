#import "MWMTypes.h"

typedef NS_ENUM(NSUInteger, MWMRestoringRequestResult)
{
  MWMRestoringRequestResultBackupExists,
  MWMRestoringRequestResultNoBackup,
  MWMRestoringRequestResultNotEnoughDiskSpace,
  MWMRestoringRequestResultNoInternet,
  MWMRestoringRequestResultRequestError
};

typedef NS_ENUM(NSUInteger, MWMSynchronizationResult)
{
  MWMSynchronizationResultSuccess,
  MWMSynchronizationResultAuthError,
  MWMSynchronizationResultNetworkError,
  MWMSynchronizationResultDiskError,
  MWMSynchronizationResultUserInterrupted,
  MWMSynchronizationResultInvalidCall
};

@protocol MWMBookmarksObserver<NSObject>
@optional
- (void)onConversionFinish:(BOOL)success;
- (void)onRestoringRequest:(MWMRestoringRequestResult)result deviceName:(NSString * _Nullable)name backupDate:(NSDate * _Nullable)date;
- (void)onSynchronizationFinished:(MWMSynchronizationResult)result;
- (void)onRestoringStarted;
- (void)onRestoringFilesPrepared;
- (void)onBookmarksLoadFinished;
- (void)onBookmarksFileLoadSuccess;
- (void)onBookmarksCategoryDeleted:(MWMMarkGroupID)groupId;
- (void)onBookmarkDeleted:(MWMMarkID)bookmarkId;
- (void)onBookmarksCategoryFilePrepared:(MWMBookmarksShareStatus)status;

@end
