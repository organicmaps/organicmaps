#import <Foundation/Foundation.h>

typedef void (^MWMVoidBlock)(void);
typedef void (^MWMStringBlock)(NSString *);
typedef void (^MWMURLBlock)(NSURL *);
typedef BOOL (^MWMCheckStringBlock)(NSString *);
typedef void (^MWMBoolBlock)(BOOL);

typedef NS_ENUM(NSUInteger, MWMDayTime) { MWMDayTimeDay, MWMDayTimeNight };

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
