//
//  MPVASTManager.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPVASTResponse.h"

@interface MPVASTManager : NSObject

+ (void)fetchVASTWithData:(NSData *)data completion:(void (^)(MPVASTResponse *, NSError *))completion;

@end
