//
//  MPVASTManager.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MPVASTResponse.h"

typedef enum {
    MPVASTErrorXMLParseFailure,
    MPVASTErrorExceededMaximumWrapperDepth,
    MPVASTErrorNoAdsFound
} MPVASTError;

@interface MPVASTManager : NSObject

+ (void)fetchVASTWithURL:(NSURL *)URL completion:(void (^)(MPVASTResponse *, NSError *))completion;
+ (void)fetchVASTWithData:(NSData *)data completion:(void (^)(MPVASTResponse *, NSError *))completion;

@end
