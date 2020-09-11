//
//  MPVASTResponse.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTResponse.h"

@interface MPVASTResponse ()

@property (nonatomic) NSArray *ads;
@property (nonatomic) NSArray *errorURLs;
@property (nonatomic, copy) NSString *version;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPVASTResponse

+ (NSDictionary *)modelMap
{
    return @{@"ads":        @[@"VAST.Ad", MPParseArrayOf(MPParseClass([MPVASTAd class]))],
             @"version":    @"VAST.version"};
}

@end
