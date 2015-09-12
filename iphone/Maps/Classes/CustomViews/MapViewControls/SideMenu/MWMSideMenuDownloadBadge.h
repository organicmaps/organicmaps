#import <UIKit/UIKit.h>

@class MWMSideMenuDownloadBadge;
@protocol MWMSideMenuDownloadBadgeOwner <NSObject>

@property (weak, nonatomic) MWMSideMenuDownloadBadge * downloadBadge;

@end

@interface MWMSideMenuDownloadBadge : UIView

@property (nonatomic) NSUInteger outOfDateCount;

- (void)showAnimatedAfterDelay:(NSTimeInterval)delay;
- (void)hide;

@end
