#import <Foundation/Foundation.h>

@interface MWMWatchNotification : NSObject

- (void)passMessageObject:(id<NSCoding>)messageObject identifier:(NSString *)identifier;
- (id)messageWithIdentifier:(NSString *)identifier;
- (void)clearMessageContentsForIdentifier:(NSString *)identifier;
- (void)clearAllMessageContents;
- (void)listenForMessageWithIdentifier:(NSString *)identifier listener:(void (^)(id messageObject))listener;
- (void)stopListeningForMessageWithIdentifier:(NSString *)identifier;

@end
