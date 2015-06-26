
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>


@interface ShareInfo : NSObject

- (instancetype)initWithText:(NSString *)text lat:(double)lat lon:(double)lon myPosition:(BOOL)myPosition;

@property (nonatomic) NSString * text;
@property (nonatomic) double lat;
@property (nonatomic) double lon;
@property (nonatomic) BOOL myPosition;

@end

@interface ShareActionSheet : NSObject

- (instancetype)initWithInfo:(ShareInfo *)info viewController:(UIViewController *)viewController;
- (void)showFromRect:(CGRect)rect;

@end
