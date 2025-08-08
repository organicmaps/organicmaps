#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageOSMContributionData : NSObject

@property(nonatomic, readonly) BOOL showAddPlace;
@property(nonatomic, readonly) BOOL showEditPlace;
@property(nonatomic, readonly) BOOL showUpdateMap;
@property(nonatomic, readonly) BOOL enableUpdateMap;

@end

NS_ASSUME_NONNULL_END
