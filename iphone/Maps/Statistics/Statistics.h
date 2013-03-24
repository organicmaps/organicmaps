#import <Foundation/Foundation.h>

@interface Statistics : NSObject

- (void) startSession;
- (void) stopSession;

+ (id) instance;

@end
