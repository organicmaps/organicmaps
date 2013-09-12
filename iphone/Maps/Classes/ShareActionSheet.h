#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface ShareActionSheet : NSObject
+(void)showShareActionSheetInView:(id)view withObject:(id)del;
+(void)resolveActionSheetChoice:(UIActionSheet *)as buttonIndex:(NSInteger)buttonIndex text:(NSString *)text
                           view:(id)view delegate:(id)del gX:(double)gX gY:(double)gY
                  andMyPosition:(BOOL)myPos;
@end
