#import <Foundation/Foundation.h>

typedef void (^MWMVoidBlock)(void);
typedef void (^MWMStringBlock)(NSString *);
typedef void (^MWMURLBlock)(NSURL *);
typedef BOOL (^MWMCheckStringBlock)(NSString *);
typedef void (^MWMBoolBlock)(BOOL);

typedef NS_ENUM(NSUInteger, MWMDayTime) { MWMDayTimeDay, MWMDayTimeNight };

typedef NS_ENUM(NSUInteger, MWMUnits) { MWMUnitsMetric, MWMUnitsImperial };

typedef NS_ENUM(NSUInteger, MWMTheme) {
  MWMThemeDay,
  MWMThemeNight,
  MWMThemeVehicleDay,
  MWMThemeVehicleNight,
  MWMThemeAuto
};

typedef uint64_t MWMMarkID;
typedef uint64_t MWMTrackID;
typedef uint64_t MWMMarkGroupID;
typedef NSArray<NSNumber *> *MWMMarkIDCollection;
typedef NSArray<NSNumber *> *MWMTrackIDCollection;
typedef NSArray<NSNumber *> *MWMGroupIDCollection;

typedef NS_ENUM(NSUInteger, MWMBookmarksShareStatus) {
  MWMBookmarksShareStatusSuccess,
  MWMBookmarksShareStatusEmptyCategory,
  MWMBookmarksShareStatusArchiveError,
  MWMBookmarksShareStatusFileError
};

typedef NS_ENUM(NSUInteger, MWMCategoryAccessStatus) {
  MWMCategoryAccessStatusLocal,
  MWMCategoryAccessStatusPublic,
  MWMCategoryAccessStatusPrivate,
  MWMCategoryAccessStatusAuthorOnly,
  MWMCategoryAccessStatusOther
};

typedef NS_ENUM(NSUInteger, MWMCategoryAuthorType) {
  MWMCategoryAuthorTypeLocal,
  MWMCategoryAuthorTypeTraveler
};

//typedef NS_ENUM(NSUInteger, MWMRestoringRequestResult)
//{
//  MWMRestoringRequestResultBackupExists,
//  MWMRestoringRequestResultNoBackup,
//  MWMRestoringRequestResultNotEnoughDiskSpace,
//  MWMRestoringRequestResultNoInternet,
//  MWMRestoringRequestResultRequestError
//};
//
//typedef NS_ENUM(NSUInteger, MWMSynchronizationResult)
//{
//  MWMSynchronizationResultSuccess,
//  MWMSynchronizationResultAuthError,
//  MWMSynchronizationResultNetworkError,
//  MWMSynchronizationResultDiskError,
//  MWMSynchronizationResultUserInterrupted,
//  MWMSynchronizationResultInvalidCall
//};
//
//@protocol MWMBookmarksObserver<NSObject>
//@optional
//- (void)onConversionFinish:(BOOL)success;
//- (void)onRestoringRequest:(MWMRestoringRequestResult)result deviceName:(NSString * _Nullable)name backupDate:(NSDate * _Nullable)date;
//- (void)onSynchronizationFinished:(MWMSynchronizationResult)result;
//- (void)onBackupStarted;
//- (void)onRestoringStarted;
//- (void)onRestoringFilesPrepared;
//- (void)onBookmarksLoadFinished;
//- (void)onBookmarksFileLoadSuccess;
//- (void)onBookmarksFileLoadError;
//- (void)onBookmarksCategoryDeleted:(MWMMarkGroupID)groupId;
//- (void)onBookmarkDeleted:(MWMMarkID)bookmarkId;
//- (void)onBookmarksCategoryFilePrepared:(MWMBookmarksShareStatus)status;
//
//@end
