//
//  MRBundleManager.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MRBundleManager : NSObject

+ (MRBundleManager *)sharedManager;
- (NSString *)mraidPath;

@end
