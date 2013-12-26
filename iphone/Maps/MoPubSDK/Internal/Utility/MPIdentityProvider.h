//
//  MPIdentityProvider.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MPIdentityProvider : NSObject

+ (NSString *)identifier;
+ (BOOL)advertisingTrackingEnabled;

@end
