#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, PlacePageOSMContributionState) {
  PlacePageOSMContributionStateCanAddOrEditPlace,
  PlacePageOSMContributionStateShouldUpdateMap,
  PlacePageOSMContributionStateMapIsDownloading,
};

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageOSMContributionData : NSObject

@property(nonatomic, readonly) BOOL showAddPlace;
@property(nonatomic, readonly) BOOL showEditPlace;

@property(nonatomic, readonly) PlacePageOSMContributionState state;

@end

NS_ASSUME_NONNULL_END
