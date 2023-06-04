#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^MWMVoidBlock)(void);
typedef void (^MWMStringBlock)(NSString *);
typedef void (^MWMURLBlock)(NSURL *);
typedef BOOL (^MWMCheckStringBlock)(NSString *);
typedef void (^MWMBoolBlock)(BOOL);

typedef NS_ENUM(NSUInteger, MWMDayTime) { MWMDayTimeDay, MWMDayTimeNight } NS_SWIFT_NAME(DayTime);

typedef NS_ENUM(NSUInteger, MWMUnits) { MWMUnitsMetric, MWMUnitsImperial } NS_SWIFT_NAME(Units);

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

typedef NS_CLOSED_ENUM(NSUInteger, MWMBookmarksShareStatus) {
  MWMBookmarksShareStatusSuccess,
  MWMBookmarksShareStatusEmptyCategory,
  MWMBookmarksShareStatusArchiveError,
  MWMBookmarksShareStatusFileError
} NS_SWIFT_NAME(BookmarksShareStatus);

typedef NS_ENUM(NSUInteger, MWMBookmarkGroupAccessStatus) {
  MWMBookmarkGroupAccessStatusLocal,
  MWMBookmarkGroupAccessStatusPublic,
  MWMBookmarkGroupAccessStatusPrivate,
  MWMBookmarkGroupAccessStatusAuthorOnly,
  MWMBookmarkGroupAccessStatusOther
} NS_SWIFT_NAME(BookmarkGroupAccessStatus);

typedef NS_ENUM(NSUInteger, MWMBookmarkGroupAuthorType) {
  MWMBookmarkGroupAuthorTypeLocal,
  MWMBookmarkGroupAuthorTypeTraveler
} NS_SWIFT_NAME(BookmarkGroupAuthorType);

typedef NS_ENUM(NSInteger, MWMBookmarkGroupType) {
  MWMBookmarkGroupTypeRoot,
  MWMBookmarkGroupTypeCategory,
  MWMBookmarkGroupTypeCollection,
  MWMBookmarkGroupTypeDay
} NS_SWIFT_NAME(BookmarkGroupType);

NS_ASSUME_NONNULL_END
