//
//  MPVASTResource.m
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTResource.h"

@implementation MPVASTResource

+ (NSDictionary *)modelMap
{
    return @{@"content":            @"text",
             @"staticCreativeType": @"creativeType"};
}

- (BOOL)isStaticCreativeTypeImage {
    return ([self.staticCreativeType.lowercaseString isEqualToString:@"image/gif"]
            || [self.staticCreativeType.lowercaseString isEqualToString:@"image/jpeg"]
            || [self.staticCreativeType.lowercaseString isEqualToString:@"image/png"]);
}

- (BOOL)isStaticCreativeTypeJavaScript {
    return [self.staticCreativeType.lowercaseString isEqualToString:@"application/x-javascript"];
}

@end
