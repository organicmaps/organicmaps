//
//  MPVASTResponse.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
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
