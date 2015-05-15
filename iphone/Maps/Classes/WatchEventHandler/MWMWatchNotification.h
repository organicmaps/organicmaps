//
//  MWMWatchNotification.h
//  Maps
//
//  Created by i.grechuhin on 10.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MWMWatchNotification : NSObject

- (void)passMessageObject:(id<NSCoding>)messageObject identifier:(NSString *)identifier;
- (id)messageWithIdentifier:(NSString *)identifier;
- (void)clearMessageContentsForIdentifier:(NSString *)identifier;
- (void)clearAllMessageContents;
- (void)listenForMessageWithIdentifier:(NSString *)identifier listener:(void (^)(id messageObject))listener;
- (void)stopListeningForMessageWithIdentifier:(NSString *)identifier;

@end
