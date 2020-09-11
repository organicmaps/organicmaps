//
//  MPURLResolver.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import "MPGlobal.h"
#import "MPURLActionInfo.h"

typedef void (^MPURLResolverCompletionBlock)(MPURLActionInfo *actionInfo, NSError *error);

@interface MPURLResolver : NSObject

+ (instancetype)resolverWithURL:(NSURL *)URL completion:(MPURLResolverCompletionBlock)completion;
- (void)start;
- (void)cancel;

@end
