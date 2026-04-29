#import <Foundation/Foundation.h>
#import <UIKit/UIColor.h>
#import "MWMTypes.h"

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageTrackSelectionData : NSObject

@property(nonatomic, readonly) MWMTrackID trackId;
@property(nonatomic, readonly) NSString * title;
@property(nonatomic, readonly) UIColor * color;
@property(nonatomic, readonly) BOOL isSelected;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
