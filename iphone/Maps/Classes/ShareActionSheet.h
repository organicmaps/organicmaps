
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>


@interface ShareInfo : NSObject

- (instancetype)initWithText:(NSString *)text gX:(double)gX gY:(double)gY myPosition:(BOOL)myPosition;

@property NSString * text;
@property double gX;
@property double gY;
@property BOOL myPosition;

@end

@interface ShareActionSheet : NSObject

- (instancetype)initWithInfo:(ShareInfo *)info viewController:(UIViewController *)viewController;
- (void)show;

@end
