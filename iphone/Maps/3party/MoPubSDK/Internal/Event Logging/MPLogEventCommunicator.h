//
//  MPLogEventCommunicator.h
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MPLogEventCommunicator : NSObject

- (void)sendEvents:(NSArray *)events;
- (BOOL)isOverLimit;

@end
