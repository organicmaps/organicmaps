//
//  NSBundle+MPAdditions.h
//  MoPubSDK
//
//  Copyright © 2016 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSBundle (MPAdditions)

/**
 * Retrieves the bundle that contains the MoPubSDK resources.
 * @param aClass MoPub class. Typically will be self.class.
 * @returns The bundle containing the MoPubSDK resources.
 */
+ (NSBundle *)resourceBundleForClass:(Class)aClass;

@end
