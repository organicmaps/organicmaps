//
//  MPURLResolver.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPGlobal.h"
#import "MPURLActionInfo.h"

typedef void (^MPURLResolverCompletionBlock)(MPURLActionInfo *actionInfo, NSError *error);

@interface MPURLResolver : NSObject <NSURLConnectionDataDelegate>

+ (instancetype)resolverWithURL:(NSURL *)URL completion:(MPURLResolverCompletionBlock)completion;
- (void)start;
- (void)cancel;

@end
