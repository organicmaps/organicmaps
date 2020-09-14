#import <Foundation/Foundation.h>

#import "MWMTypes.h"
NS_ASSUME_NONNULL_BEGIN

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

NS_SWIFT_NAME(BookmarksObserver)
@protocol MWMBookmarksObserver<NSObject>
@optional
- (void)onConversionFinish:(BOOL)success;
- (void)onRestoringRequest:(MWMRestoringRequestResult)result
                deviceName:(NSString * _Nullable)name
                backupDate:(NSDate * _Nullable)date;
- (void)onSynchronizationFinished:(MWMSynchronizationResult)result;
- (void)onBackupStarted;
- (void)onRestoringStarted;
- (void)onRestoringFilesPrepared;
- (void)onBookmarksLoadFinished;
- (void)onBookmarksFileLoadSuccess;
- (void)onBookmarksFileLoadError;
- (void)onBookmarksCategoryDeleted:(MWMMarkGroupID)groupId;
- (void)onBookmarkDeleted:(MWMMarkID)bookmarkId;
- (void)onBookmarksCategoryFilePrepared:(MWMBookmarksShareStatus)status;

@end
NS_ASSUME_NONNULL_END
